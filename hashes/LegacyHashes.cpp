/*
 * SMHasher3
 * Copyright (C) 2021-2022  Frank J. T. Wojcik
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *     Copyright (c) 2010-2012 Austin Appleby
 *     Copyright (c) 2014-2021 Reini Urban
 *     Copyright (c) 2015      Ivan Kruglov
 *     Copyright (c) 2015      Paul G
 *     Copyright (c) 2016      Jason Schulz
 *     Copyright (c) 2016-2018 Leonid Yuriev
 *     Copyright (c) 2016      Sokolov Yura aka funny_falcon
 *     Copyright (c) 2016      Vlad Egorov
 *     Copyright (c) 2018      Jody Bruchon
 *     Copyright (c) 2019      Niko Rebenich
 *     Copyright (c) 2019-2020 Yann Collet
 *     Copyright (c) 2019-2021 data-man
 *     Copyright (c) 2019      王一 WangYi
 *     Copyright (c) 2020      Cris Stringfellow
 *     Copyright (c) 2020      HashTang
 *     Copyright (c) 2020      Jim Apple
 *     Copyright (c) 2020      Thomas Dybdahl Ahle
 *     Copyright (c) 2020      Tom Kaitchuck
 *     Copyright (c) 2021      Logan oos Even
 *
 *     Permission is hereby granted, free of charge, to any person
 *     obtaining a copy of this software and associated documentation
 *     files (the "Software"), to deal in the Software without
 *     restriction, including without limitation the rights to use,
 *     copy, modify, merge, publish, distribute, sublicense, and/or
 *     sell copies of the Software, and to permit persons to whom the
 *     Software is furnished to do so, subject to the following
 *     conditions:
 *
 *     The above copyright notice and this permission notice shall be
 *     included in all copies or substantial portions of the Software.
 *
 *     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *     EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *     OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *     NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *     HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *     WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *     FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *     OTHER DEALINGS IN THE SOFTWARE.
 */
#include "Platform.h"
#include "Types.h"
#include "Hashes.h"
#include "LegacyHashes.h"
#include "VCode.h"

hash_state ltc_state;

// sorted by quality and speed. the last is the list of internal secrets to be tested against bad seeds.
// marked with !! are known bad seeds, which either hash to 0 or create collisions.
static LegacyHashInfo g_hashes[] =
{
 // here start the real hashes. first the problematic ones:
#ifdef HAVE_BIT32
 #define FIBONACCI_VERIF      0x09952480
 #define FNV2_VERIF           0x739801C5
#else
 #define FIBONACCI_VERIF      0xFE3BD380
 #define FNV2_VERIF           0x1967C625
#endif

#ifdef __SIZEOF_INT128__
  // M. Dietzfelbinger, T. Hagerup, J. Katajainen, and M. Penttonen. A reliable randomized
  // algorithm for the closest-pair problem. J. Algorithms, 25:19–51, 1997.
  { multiply_shift,       64, 0xFCE355A6, "multiply_shift", "Dietzfelbinger Multiply-shift on strings", POOR,
    { 0xfffffff0, 0x1fffffff0, 0xb13dea7c9c324e51ULL, 0x75f17d6b3588f843ULL } /* !! all & 0xfffffff0 (2^32 bad seeds) */ },
  { pair_multiply_shift,  64, 0xD4B20347, "pair_multiply_shift", "Pair-multiply-shift", POOR,
    { 0xb13dea7c9c324e51ULL, 0x75f17d6b3588f843ULL } },
#endif
  { crc32,                32, 0x3719DB20, "crc32",       "CRC-32 soft", POOR, {} },

  // totally broken seed mixin
  { sha2_224,            224, 0x407AA518, "sha2-224",     "SHA2-224", GOOD, {0xc1059ed8} /* >100 bad seeds */ },
  { sha2_224_64,          64, 0xF3E40ECA, "sha2-224_64",  "SHA2-224, low 64 bits", GOOD, {0xc1059ed8} },
  { sha2_256,            256, 0xEBDA2FB1, "sha2-256",     "SHA2-256", POOR, {0x6a09e667} },
  { sha2_256_64,          64, 0xC1C4FA72, "sha2-256_64",  "SHA2-256, low 64 bits", POOR, {0x6a09e667} },
#if defined(HAVE_SHANI) && defined(__x86_64__)
  { sha2ni_256,          256, 0x4E3BB25E, "sha2ni-256",   "SHA2_NI-256 (amd64 HW SHA ext)", POOR, {0x6a09e667} },
  { sha2ni_256_64,        64, 0xF938E80E, "sha2ni-256_64","hardened SHA2_NI-256 (amd64 HW SHA ext), low 64 bits", POOR, {0x6a09e667} },
#endif
  { rmd128,              128, 0xFF576977, "rmd128",       "RIPEMD-128", GOOD, {0x67452301} },
  { rmd160,              160, 0x30B37AC6, "rmd160",       "RIPEMD-160", GOOD, {0x67452301} },
  { rmd256,              256, 0xEB16FAD7, "rmd256",       "RIPEMD-256", GOOD, {0x67452301} },
#if defined(HAVE_BIT32) && !defined(_WIN32)
#  define BLAKE3_VERIF   0x58571F56
#else
#  define BLAKE3_VERIF   0x50E4CD91
#endif
  { blake3c_test,        256, BLAKE3_VERIF, "blake3_c",   "BLAKE3 c",   GOOD, {0x6a09e667} },
#if defined(HAVE_BLAKE3)
  { blake3_test,         256, 0x0, "blake3",       "BLAKE3 Rust", GOOD, {} },
  { blake3_64,            64, 0x0, "blake3_64",    "BLAKE3 Rust, low 64 bits", GOOD, {} },
#endif
  { blake2s128_test,     128, 0xE8D8FCDF, "blake2s-128",  "blake2s-128", GOOD, {0x6a09e667} },
  { blake2s160_test,     160, 0xD50FF144, "blake2s-160",  "blake2s-160", GOOD, {0x6a09e667} },
  { blake2s224_test,     224, 0x19B36D2C, "blake2s-224",  "blake2s-224", GOOD, {0x6a09e667} },
  { blake2s256_test,     256, 0x841D6354, "blake2s-256",  "blake2s-256", GOOD,
    {0x31, 0x32, 0x15e, 0x432, 0x447, 0x8000001e, 0x80000021 } /* !! and >1000 more */ },
  { blake2s256_64,        64, 0x53000BB2, "blake2s-256_64","blake2s-256, low 64 bits", GOOD,
    {0xa, 0xe, 0x2d, 0x2f, 0x53, 0x40000003, 0x40000005, 0x40000006 } /* !! and >1000 more */ },
  { blake2b160_test,     160, 0x28ADDA30, "blake2b-160",  "blake2b-160", GOOD,
    {0x4a, 0x5a, 0x5e, 0x74, 0x7f, 0x81} /* !! and >1000 more */ },
  { blake2b224_test,     224, 0x101A62A4, "blake2b-224",  "blake2b-224", GOOD,
    {0x12, 0x2e, 0x32, 0x99a, 0xc80, 0xc98, 0xc9c} /* !! and >1000 more */ },
  { blake2b256_test,     256, 0xC9D8D995, "blake2b-256",  "blake2b-256", POOR, {} },
  { blake2b256_64,        64, 0xCF4F7EC3, "blake2b-256_64","blake2b-256, low 64 bits", GOOD, {} },
  { asconhashv12_256,    256, 0xA969C160, "asconhashv12", "asconhashv12 256bit", GOOD,
    { 0xee9398aadb67f03dULL } },
  { asconhashv12_64,      64, 0xE7DEF300, "asconhashv12_64", "asconhashv12, low 64 bits", GOOD,
    { 0xee9398aadb67f03dULL } },
  { sha3_256,            256, 0x21048CE3, "sha3-256",     "SHA3-256 (Keccak)", GOOD, {0x1UL} },
  { sha3_256_64,          64, 0xE62E5CC0, "sha3-256_64",  "SHA3-256 (Keccak), low 64 bits", GOOD, {0x1UL} },
#if defined(HAVE_SSE2)
  { hasshe2_test,        256, 0xF5D39DFE, "hasshe2",     "SSE2 hasshe2, 256-bit", POOR, {} },
#endif
// too fragile
#ifdef __SIZEOF_INT128__
#ifdef __FreeBSD__
#  define POLY1_VERIF   0xFECCC371
#  define POLY2_VERIF   0xA31DD921
#  define POLY3_VERIF   0x26F7EDA0
#  define POLY4_VERIF   0x8EE270BD
#  define TABUL_VERIF   0x0534C36E
#elif defined __apple_build_version__ && defined __clang__
#  define POLY1_VERIF   0xE389931F
#  define POLY2_VERIF   0x458D056D
#  define POLY3_VERIF   0x208145CC
#  define POLY4_VERIF   0xE798712E
#  define TABUL_VERIF   0x91618263
#elif defined DEBUG
#  define POLY1_VERIF   0x9E4BA93D
#  define POLY2_VERIF   0x5CB4CB25
#  define POLY3_VERIF   0x3C87C852
#  define POLY4_VERIF   0xFF88BAF6
#  define TABUL_VERIF   0xB49C607C
#else
#  define POLY1_VERIF   0x64706572
#  define POLY2_VERIF   0xE8906EDF
#  define POLY3_VERIF   0xF2A7E178
#  define POLY4_VERIF   0xD4E89421
#  define TABUL_VERIF   0xB49C607C
#endif
  // Thomas Dybdahl Ahle, Jakob Tejs Bæk Knudsen, and Mikkel Thorup
  // "The Power of Hashing with Mersenne Primes".
  { poly_1_mersenne,      32, 0, "poly_1_mersenne", "Degree 1 Hashing mod 2^61-1", POOR, {} },
  { poly_2_mersenne,      32, 0, "poly_2_mersenne", "Degree 2 Hashing mod 2^61-1", GOOD, {0x60e8512c} /* !! */},
  { poly_3_mersenne,      32, 0, "poly_3_mersenne", "Degree 3 Hashing mod 2^61-1", GOOD, {0x3d25f745} /* !! */},
  { poly_4_mersenne,      32, 0, "poly_4_mersenne", "Degree 4 Hashing mod 2^61-1", GOOD, {} },
  { tabulation_test,      64, TABUL_VERIF, "tabulation",      "64-bit Tabulation with Multiply-Shift Mixer", GOOD, {} },
#endif
#if defined(_MSC_VER) /* truncated long to 32 */
#  define TABUL32_VERIF   0x3C3B7BDD
#elif defined __FreeBSD__
#  define TABUL32_VERIF   0x4D28A619
#elif defined __apple_build_version__ && defined __clang__
#  define TABUL32_VERIF   0x2C8EDFFE
#else
#  define TABUL32_VERIF   0x335F64EA
#endif
  { tabulation_32_test,   32, TABUL32_VERIF, "tabulation32",    "32-bit Tabulation with Multiply-Shift Mixer", POOR, {} },
#ifdef HAVE_SSE42
  // all CRC's are insecure by default due to its polynomial nature.
  /* Even 32 uses crc32q, quad only */
#  if defined(_MSC_VER) /* truncated long to 32? */
#   define CRC32_VERIF   0xC2B84071
#   define CRC64_VERIF   0x6BBC19D6
#  else  
#   define CRC32_VERIF   0x0C7346F0
#   define CRC64_VERIF   0xE7C3FD0E
#endif
#ifndef HAVE_BROKEN_MSVC_CRC32C_HW
  { crc32c_hw_test,       32, CRC32_VERIF, "crc32_hw",    "SSE4.2 crc32 in HW", POOR, {0x111c2232} /* !! */},
  { crc64c_hw_test,       64, CRC64_VERIF, "crc64_hw",    "SSE4.2 crc64 in HW", POOR, {0x0} /* !! */ },
#endif
#if defined(__SSE4_2__) && (defined(__i686__) || defined(_M_IX86) || defined(__x86_64__))
  { crc32c_hw1_test,      32, 0x0C7346F0,  "crc32_hw1",   "Faster Adler SSE4.2 crc32 on Intel HW", POOR, {0x111c2232} /* !! */},
#endif
#endif
  // 32bit crashes
#ifdef HAVE_INT64
  { o1hash_test,          64, 0x85051E87, "o1hash",       "o(1)hash unseeded, from wyhash", POOR, {0x0} /* !! */ },
#endif
#if 0 && defined(__x86_64__) && (defined(__linux__) || defined(__APPLE__))
  // elf64 or macho64 only
  { fhtw_test,            64, 0x0,        "fhtw",        "fhtw asm", POOR, {} },
#endif
  { fibonacci_test, __WORDSIZE, FIBONACCI_VERIF, "fibonacci",   "wordwise Fibonacci", POOR,
    {0x0, 0xffffffff00000000ULL} /* !! all keys ending with 0x0000_0000 */ },
#ifndef HAVE_ALIGNED_ACCESS_REQUIRED
  { khash32_test,         32, 0x99B3FFCD, "k-hash32",    "K-Hash mixer, 32-bit", POOR, {0,1,2,3,5,0x40000001} /*... !!*/},
  { khash64_test,         64, 0xAB5518A1, "k-hash64",    "K-Hash mixer, 64-bit", POOR, {0,1,2,3,4,5} /*...!!*/},
#endif
  { FNV32a_test,          32, 0xE3CBBE91, "FNV1a",       "Fowler-Noll-Vo hash, 32-bit", POOR,
    {0x811c9dc5} /* !! */ },
#ifdef HAVE_INT64
  { FNV1A_Totenschiff_test,32,0x95D95ACF, "FNV1A_Totenschiff",  "FNV1A_Totenschiff_v1 64-bit sanmayce", POOR,
    {0x811c9dc5} },
  { FNV1A_PY_test,        32, 0xE79AE3E4, "FNV1A_Pippip_Yurii", "FNV1A-Pippip_Yurii 32-bit sanmayce", POOR,
    {0x811c9dc5} },
  { FNV32a_YT_test,       32, 0xD8AFFD71, "FNV1a_YT",    "FNV1a-YoshimitsuTRIAD 32-bit sanmayce", POOR,
    {0x811c9dc5, 0x23d4a49d} /* !! */ },
  { FNV64a_test,          64, 0x103455FC, "FNV64",       "Fowler-Noll-Vo hash, 64-bit", POOR,
    {0x811c9dc5, 0xcbf29ce4, 0x84222325, 0xcbf29ce484222325} /* TODO */},
#endif
  { FNV2_test,    __WORDSIZE, FNV2_VERIF, "FNV2",        "wordwise FNV", POOR, {} },
  { fletcher2_test,       64, 0x890767C0, "fletcher2",   "fletcher2 ZFS", POOR, {0UL} /* !! */ },
  { fletcher4_test,       64, 0x47660EB7, "fletcher4",   "fletcher4 ZFS", POOR, {0UL} /* !! */ },
  { Bernstein_test,       32, 0xBDB4B640, "bernstein",   "Bernstein, 32-bit", POOR, {0UL} /* !! */ },
  { sdbm_test,            32, 0x582AF769, "sdbm",        "sdbm as in perl5", POOR, {0UL} /* !! */ },
  { x17_test,             32, 0x8128E14C, "x17",         "x17", POOR, {} },
  // also called jhash:
  { JenkinsOOAT_test,     32, 0x83E133DA, "JenkinsOOAT", "Bob Jenkins' OOAT as in perl 5.18", POOR, {0UL} /* !! */ },
  { JenkinsOOAT_perl_test,32, 0xEE05869B, "JenkinsOOAT_perl", "Bob Jenkins' OOAT as in old perl5", POOR, {0UL} /* !! */},
#if defined(_MSC_VER) && defined(LTO)
#  define BEBB4185_VERIF          0xB7013C8F
#else
#  define BEBB4185_VERIF          0xBEBB4185
#endif
#ifndef HAVE_ALIGNED_ACCESS_REQUIRED
  { BEBB4185_64,          64, BEBB4185_VERIF, "BEBB4185", "BEBB4185 64", POOR,
    { /* and many more */
      0x1001ee, 0x10056d, 0x100ea7, 0x10270c, 0x105b31, 0x105b45, 0x10624e,
      0x106ab9, 0x106b29, 0x1070d4, 0x1073d5, 0x10798a, 0x108973, 0x108fda,
      0x10aac7, 0x10adb8, 0x10af83, 0x10ba9a, 0x10c3f8, 0x1100e, 0x1115d6,
      0x1128af, 0x112936, 0x113509, 0x11440e, 0x114efa, 0x1150af, 0x119311,
      0x119385, 0x119803, 0x11a6ab, 0x11a936, 0x11aa17, 0x11abbd, 0x11c93,
      0x11d2d, 0x11dc2e, 0x11e043, 0x11e824, 0x12374f, 0x123b91, 0x123d66,
      0x123ff4, 0x1240a0, 0x128ac3, 0x12d1d9, 0x12e995, 0x12f5d4, 0x1304df,
      0x1307d4, 0x1384b3, 0x1384bd, 0x138a18, 0x13e21, 0x13e7d3, 0x13ec3,
      0x13fc5b, 0x145e34, 0x146acc, 0x147a10, 0x14810c, 0x149013, 0x14a00d,
      0x14a2f7, 0x14be23, 0x14e138, 0x14e2ce, 0x14e6d5, 0x14e875, 0x14e96a,
      0x14ea15, 0x153e42, 0x1543f2, 0x155ade, 0x156240, 0x156a09, 0x156e70,
      0x157501, 0x157559, 0x15d387, 0x15ee8b, 0x15ef4b, 0x162a21, 0x162a90,
      0x163bcb, 0x16405d, 0x164582, 0x164663, 0x166d59, 0x1672d5, 0x167779,
      0x167e80, 0x169017, 0x16c216, 0x16c2ac, 0x16d1c6, 0x16d373, 0x16d54d,
      0x16d7d0, 0x16dae3, 0x16e31a, 0x16ea2c, 0x16eb10, 0x16f044, 0x16f0b9,
      0x16f412, 0x171b2d, 0x17256c, 0x172667, 0x1730b3, 0x1730ed, 0x1738a9,
      0x1757ab, 0x1774d7, 0x1781fb, 0x1795a5, 0x179ac1, 0x179ba9, 0x17a445,
      0x17bbe1, 0x17ce50, 0x180a39, 0x180ac1, 0x182bfd, 0x183e53, 0x184020,
      0x184404, 0x184549, 0x1874f3, 0x1876b4, 0x1881ca, 0x18876f, 0x188a6b,
      0x1890a9, 0x1897fa, 0x18ad35, 0x1943d, 0x19fd2, 0x1a07e, 0x1a0eb,
      0x1f89e, 0x1fcf8, 0x22a35, 0x22ba9, 0x22c73, 0x23eca, 0x25df1, 0x2672f,
      0x26a39, 0x285c8, 0x288a2, 0x28995, 0x2db44, 0x2de4f, 0x2de83, 0x2e09d,
      0x32fa6, 0x331fe, 0x33dd4, 0x346ce, 0x35208, 0x38a3d, 0x38b26, 0x38be5,
      0x39148, 0x39a88, 0x3ac69, 0x3ad16, 0x3ad9e, 0x3b792, 0x3b807, 0x3b8a5,
      0x3ba55, 0x3bc21, 0x3bcbc, 0x3bd8, 0x3c898, 0x3ca40, 0x40000d6c,
      0x40004cff, 0x40004ec8, 0x40005c11, 0x40006297, 0x400064dd, 0x4000781c,
      0x40007a98, 0x40007c11, 0x40007df8, 0x40007f4b, 0x40008043, 0x4000870f,
      0x40008e12, 0x40008e62, 0x40008f70, 0x40009688, 0x400096d6, 0x4000a4eb,
      0x4000a60b, 0x4000a8f2, 0x4000abbf, 0x4000b237, 0x4000b5aa, 0x4000b6e7,
      0x4000b70a, 0x4000b968, 0x4000bbc9, 0x4000d26b, 0x4000d338, 0x4000d3be,
      0x4000d462, 0x4000db50, 0x4000deac, 0x4000e7f3, 0x4000e89b, 0x40011791,
      0x40013562, 0x40013569, 0x40013bf1, 0x4001421d, 0x4001470a, 0x4001476d,
      0x400156f8, 0x4001574e, 0x40017b5d, 0x40017b7a, 0x40018e70, 0x40018eed,
      0x400190f7, 0x400196be, 0x40019b5f, 0x4001b415, 0x4001b73f, 0x4001d2bf,
      0x4001d407, 0x4001d8fb, 0x4001daff, 0x4001ea14, 0x400213dd, 0x40022633,
      0x4002326a, 0x40023723, 0x4002380e, 0x40023ac7, 0x40023c52, 0x4002410f,
      0x40026393, 0x4002734a, 0x4002769e, 0x40027ba0, 0x40027f3e, 0x4002b817,
      0x4002bbc9, 0x4002d326, 0x4002d9e4, 0x4002da76, 0x4002fe74, 0x40030f55,
      0x40031d77, 0x400324a6, 0x40033b5c, 0x4003a16a, 0x4003dfcc, 0x4003e225,
      0x4003f439, 0x40058821, 0x40061129, 0x400617b5, 0x400618a2, 0x40061993,
      0x40062f36, 0x40063113, 0x40064122, 0x40064ce8, 0x40065358, 0x400710bc,
      0x4009663f, 0x400a2441, 0x400a286e, 0x400b476c, 0x400b49a4, 0x400b5ce3,
      0x400b69ac, 0x400b7016, 0x400b9145, 0x400b9193, 0x400b988e, 0x400bc99f,
      0x400bcfae, 0x400bd042, 0x400bd581, 0x400bd58b, 0x400bddf8, 0x400c02f3,
      0x400c03df, 0x400c0504, 0x400c05e6, 0x400c07a9, 0x400c0b1a, 0x400c0c31,
      0x400c0e2f, 0x400c0fd6, 0x400c1065, 0x400c1c5a, 0x400c1d08, 0x400c2175,
      0x400c2a3f, 0x400c2bc0, 0x400c2c14, 0x400c2e43, 0x400c2fb7, 0x400c3082,
      0x400c32e0, 0x400ccadd, 0x400cd0e5, 0x400cd835, 0x400cdcf8, 0x400cf107,
      0x400cf8fc, 0x400cfc1b, 0x400cfc28, 0x400d0a49, 0x400d2e92, 0x400d2f4b,
      0x400d35cc, 0x400d456c, 0x400d4793, 0x400d6e69, 0x400d6f64, 0x400d730c,
      0x400d767c, 0x400d78a2, 0x400d80aa, 0x400d86d4, 0x400d8a9d, 0x400d8d03,
      0x400d8df8, 0x400d8f67, 0x400d8ffc, 0x400d90b2, 0x400dc737, 0x400dee67,
      0x400e09f8, 0x400e0a76, 0x400e1eef, 0x400e26fb, 0x400e279e, 0x400e2b71,
      0x400e30dc, 0x400e7cc0, 0x400e934c, 0x400ea154, 0x400ead2a, 0x400ec428,
      0x400ed4eb, 0x400ee5f7, 0x400ef55c, 0x400f3116, 0x400f31f9, 0x400f8e28,
      0x400f9ca2, 0x400f9cc3, 0x400fb3b1, 0x400fb8ac, 0x400fb8af, 0x400fc968,
      0x400fcd32, 0x40102e7a, 0x40105081, 0x40105274, 0x40106ce3, 0x40107b18,
      0x40107d59, 0x40107fe0, 0x4010802c, 0x40108113, 0x40108655, 0x4010c154,
      0x4011179f, 0x40112701, 0x401127af, 0x40115ecd, 0x40115f95, 0x4011610a,
      0x40117329, 0x4011894d, 0x40118cb6, 0x4011a012, 0x4011a3d3, 0x4011a7f9,
      0x4011b191, 0x40124f69, 0x401256f9, 0x40125762, 0x401276ff, 0x4012b16a,
      0x4012c28b, 0x4012c7ab, 0x4012c7b2, 0x4012cc8b, 0x4012fae7, 0x40133f99,
      0x40135296, 0x4013569b, 0x40135888, 0x4013599c, 0x40136dcd, 0x401377b4,
      0x40138038, 0x40138551, 0x40138bb7, 0x4013ae83, 0x4013b286, 0x4013b409,
      0x4013b57a, 0x4013b7bb, 0x4013b98d, 0x40141361, 0x401413d3, 0x40141494,
      0x40141b19, 0x40142e73, 0x40144520, 0x4014529f, 0x401463fb, 0x401469aa,
      0x40146a8d, 0x4014b1fb, 0x4014b679, 0x4014c2d1, 0x4014cfad, 0x4014d54a,
      0x4014e8c9, 0x4014fbaf, 0x401507fa, 0x401530e2, 0x40156f33, 0x40158118,
      0x4015826b, 0x40158d54, 0x40158d91, 0x4015a296, 0x4015a61b, 0x4015a9f3,
      0x4015b074, 0x4015b4a4, 0x4015c4d0, 0x4015cbcc, 0x4015cd11, 0x4015d79e,
      0x4015d8a5, 0x4015ddad, 0x4015e38e, 0x4015e927, 0x4015ef23, 0x4015f372,
      0x4015f52d, 0x40163d60, 0x401652c3, 0x40165732, 0x401659bb, 0x40165aeb,
      0x40167a5b, 0x401691a9, 0x4016948f, 0x40169825, 0x4016f6ca, 0x4016faae,
      0x4016fc7a, 0x4016fd31, 0x4016fdaf, 0x4017074f, 0x4017084a, 0x40179631,
      0x401796ed, 0x401799e4, 0x40179b56, 0x4017a6fe, 0x4017b699, 0x4017ba94,
      0x422ec, 0x49cd, 0x49ff, 0x4be83, 0x4bfb5, 0x4c570, 0x4cd61, 0x4d3a9,
      0x4de36, 0x4e30b, 0x4ee8a, 0x6271d, 0x63923, 0x78a8, 0x79c1, 0x7def,
      0x80001895, 0x80001ec9, 0x80002a60, 0x80002d7b, 0x80002fb4, 0x800042ba,
      0x80004727, 0x8000473b, 0x80004879, 0x80004c50, 0x80004ffe, 0x80005689,
      0x800056c6, 0x800058cd, 0x80007034, 0x800071e7, 0x8000749b, 0x80007623,
      0x80007654, 0x80007acb, 0x80007f68, 0x8000b154, 0x8000b1d0, 0x8000b3eb,
      0x8000b420, 0x8000b433, 0x8000b665, 0x8000bfcb, 0x8000c038, 0x8000c107,
      0x8000c3de, 0x8000c8a2, 0x8000caa3, 0x8000d18d, 0x8000d3c5, 0x8000d595,
      0x8000e778, 0x8000e8a4, 0x8000f6cb, 0x8001191e, 0x80011c97, 0x800120d0,
      0x80012c36, 0x80019b2e, 0x80019e18, 0x8001b5f9, 0x8001bf9a, 0x8001ca66,
      0x8001e32f, 0x8001e47c, 0x8001eb9d, 0x800205fb, 0x8002c60e, 0x8002c8cb,
      0x8002ce0d, 0x8002d3b8, 0x8002d43f, 0x8002ddaf, 0x8002e521, 0x8002e855,
      0x8002f005, 0x8002f8a0, 0x8002fbaa, 0x80040b70, 0x80040bf9, 0x800430d8,
      0x8004371f, 0x8004377b, 0x80043ac8, 0x80045d38, 0x80045ecd, 0x80052ae2,
      0x80052e0f, 0x800532ba, 0x8005d6e0, 0x800801d8, 0x80085e74, 0x8008b912,
      0x80091ae4, 0x80094f32, 0x8009f19d, 0x8009f585, 0x8009f587, 0x8009f732,
      0x8009f9af, 0x800a096a, 0x800a0c22, 0x800a252a, 0x800a3ab1, 0x800a3b7f,
      0x800a3eac, 0x800a4197, 0x800a4251, 0x800a4413, 0x800a51ea, 0x800a5f55,
      0x800a6cb3, 0x800aaaca, 0x800ab02a, 0x800ab0d6, 0x800ab286, 0x800abd5d,
      0x800abf4c, 0x800ac9cc, 0x800afa75, 0x800afcc4, 0x800b03ee, 0x800b3345,
      0x800b4deb, 0x800b8370, 0x800b8497, 0x800b9276, 0x800b975c, 0x800b97b0,
      0x800c35fb, 0x800c3661, 0x800c4830, 0x800c6498, 0x800d093e, 0x800d13a1,
      0x800d2486, 0x800d260d, 0x800d2bd5, 0x800d3fd1, 0x800d40a3, 0x800d4382,
      0x800d5e75, 0x800d8121, 0x800d963c, 0x800d97ef, 0x800d9b10, 0x800d9c8e,
      0x800db325, 0x800db61e, 0x800dc561, 0x800de0be, 0x800de531, 0x800df3f1,
      0x800dfa04, 0x800e69b9, 0x800e937a, 0x800e93f1, 0x800e9caf, 0x800e9cbc,
      0x800eaa9d, 0x800ebc56, 0x800ec0c6, 0x800ec41e, 0x800f1ad4, 0x800f1b91,
      0x800f23de, 0x800f23fa, 0x800f9fa1, 0x800fa0ca, 0x800fa384, 0x800fa8ff,
      0x800fab7e, 0x800faba2, 0x800fac75, 0x800faca3, 0x800fadaa, 0x800fadc5,
      0x800fb065, 0x800fb802, 0x800fd8bc, 0x801001df, 0x80100c3c, 0x801037d6,
      0x8010397f, 0x80103a5e, 0x80103aa6, 0x80103c31, 0x80103efe, 0x8010424a,
      0x80104449, 0x8010735a, 0x801083f5, 0x8010d6e2, 0x8010e2a1, 0x8010e61b,
      0x80110c4c, 0x8011188b, 0x80114eb9, 0x80114f03, 0x801150f6, 0x801155f5,
      0x8011576b, 0x8011613e, 0x80117430, 0x801175ba, 0x80118be1, 0x80118c2f,
      0x80119181, 0x8011a615, 0x80121629, 0x80121ad3, 0x80121d8a, 0x80121e1e,
      0x8012284d, 0x8012287a, 0x8012345f, 0x80123626, 0x80123b2c, 0x80123cc0,
      0x80123ed4, 0x801250a5, 0x801259e9, 0x80125ec1, 0x801265d3, 0x80128803,
      0x8012ffb9, 0x80130aaa, 0x80130afc, 0x8013123a, 0x80131c25, 0x80132522,
      0x80133f6a, 0x80134384, 0x8013704e, 0x801370e1, 0x801374b8, 0x80137694,
      0x80138184, 0x801387cd, 0x80139198, 0x80139a18, 0x8013ba63, 0x8013bb98,
      0x8013bcbc, 0x8013cf36, 0x801417a2, 0x80143b4b, 0x80143db8, 0x80144c6d,
      0x80145840, 0x80145c0f, 0x801462c7, 0x801471fd, 0x8014825e, 0x80148be8,
      0x80148d1b, 0x8014b084, 0x8014b0d8, 0x8014dc44, 0x8014e819, 0x8014fd66,
      0x8015049f, 0x80151666, 0x80151cc0, 0x80151e87, 0x801537cb, 0x80154344,
      0x801552b3, 0x801553b0, 0x80155617, 0x80155c5f, 0x80159fed, 0x8015f8e5,
      0x8015f969, 0x801643e2, 0x801671a6, 0x80169d3f, 0x8016a7c3, 0x8016aad4,
      0x8016b41c, 0x82cdf, 0x8d3c5, 0x8d3ff, 0x926e, 0xa07fb, 0xa0f19, 0xa2f0,
      0xac0f, 0xb10b5, 0xb1596, 0xb1ae5, 0xb1d9b, 0xb293, 0xb691, 0xb7cb,
      0xba972, 0xbae83, 0xbb7d1, 0xbb82e, 0xbc3e8, 0xbc78, 0xbc800, 0xc218f,
      0xc257e, 0xc2c8, 0xc3185, 0xc3b65, 0xc3d8b, 0xc3fc3, 0xc4041, 0xc422,
      0xc70c, 0xcc904, 0xccb10, 0xccf77, 0xcd2b5, 0xcdbda, 0xce1ac, 0xce2e9,
      0xce44f, 0xcf131, 0xcf5cf, 0xcf63e, 0xcf666, 0xd1e2d, 0xd432a, 0xd4649,
      0xd484c, 0xd4927, 0xd4bf8, 0xd4f6b, 0xd6c40, 0xd7971, 0xd8c2, 0xdc0c1,
      0xdc278, 0xdc33e, 0xdc4f8, 0xdc6db, 0xdd288, 0xdd4c, 0xe048, 0xe10e,
      0xe1ad7, 0xe1b01, 0xe1cc6, 0xe323f, 0xe33e2, 0xe35b6, 0xe36a5, 0xe483,
      0xe5cb, 0xe66dd, 0xe8274, 0xea68d, 0xea6a7, 0xea844, 0xea87d, 0xeab86,
      0xeab8e, 0xeaca9, 0xeacc, 0xeafc8, 0xeb620, 0xebca5, 0xebd4e, 0xee285,
      0xee48a, 0xf13a7, 0xf260f, 0xf4b4e, 0xf5726, 0xf6985, 0xf6a38, 0xf6a64,
      0xf6f50, 0xf707e, 0xf70db, 0xfaacc, 0xfab7b, 0xfb1a8, 0xfb5dd, 0xfb9ed,
      0xfbae4, 0xfde9e, /* ... */} /* !! too many */},
#endif
#if defined(HAVE_SSE42) && defined(__x86_64__) && !defined(_MSC_VER)
  // empty verify with msvc. avoid
  { pearson64_test,       64, 0x12E4C8CD, "pearsonhash64",    "Pearson hash, 64-bit SSSE3", POOR, {}},
  { pearson128_test,     128, 0x6CCBB7B3, "pearsonhash128",   "Pearson hash, 128-bit SSSE3, low 64-bit", POOR, {}},
  { pearson256_test,     256, 0x7F8BEB21, "pearsonhash256",   "Pearson hash, 256-bit SSSE3, low 64-bit", POOR, {}},
#endif
#ifdef HAVE_INT64
  { pearsonb64_test,      64, 0xB6FF2DFC, "pearsonbhash64",  "Pearson block hash, 64-bit", GOOD, {}},
  { pearsonb128_test,    128, 0x6BEFE6EA, "pearsonbhash128", "Pearson block hash, 128-bit, low 64-bit", GOOD, {}},
  { pearsonb256_test,    256, 0x999B3C19, "pearsonbhash256", "Pearson block hash, 256-bit, low 64-bit", GOOD, {}},
#endif
  // FIXME: seed
#ifdef __aarch64__
  #define VHASH32_VERIF 0x0F02AEFD
  #define VHASH64_VERIF 0xFAAEE597
#else
  #define VHASH32_VERIF 0xF0077651
  #define VHASH64_VERIF 0xF97D84FE
#endif
  { VHASH_32,             32, VHASH32_VERIF, "VHASH_32",    "VHASH_32 by Ted Krovetz and Wei Dai", POOR, {} },
  { VHASH_64,             64, VHASH64_VERIF, "VHASH_64",    "VHASH_64 by Ted Krovetz and Wei Dai", POOR, {} },
  { MicroOAAT_test,       32, 0x16F1BA97,    "MicroOAAT",   "Small non-multiplicative OAAT (by funny-falcon)", POOR,
    {0x3b00} },
#ifdef HAVE_SSE2
  { farsh32_test,         32, 0xBCDE332C, "farsh32",     "FARSH 32bit", POOR, {} }, // insecure
  { farsh64_test,         64, 0xDE2FDAEE, "farsh64",     "FARSH 64bit", POOR, {} }, // insecure
  { farsh128_test,       128, 0x82B6CBEC, "farsh128",    "FARSH 128bit", POOR, {} },
  { farsh256_test,       256, 0xFEBEA0BC, "farsh256",    "FARSH 256bit", POOR, {} },
#endif
  { jodyhash32_test,      32, 0xFB47D60D, "jodyhash32",  "jodyhash, 32-bit (v5)", POOR, {} },
#ifdef HAVE_INT64
  { jodyhash64_test,      64, 0x9F09E57F, "jodyhash64",  "jodyhash, 64-bit (v5)", POOR, {} },
#endif
  { lookup3_test,         32, 0x3D83917A, "lookup3",     "Bob Jenkins' lookup3", POOR, {0x21524101} /* !! */},
#ifdef __aarch64__
  #define SFAST_VERIF 0x6306A6FE
#else
  #define SFAST_VERIF 0x0C80403A
#endif
  { SuperFastHash_test,   32, SFAST_VERIF,"superfast",   "Paul Hsieh's SuperFastHash", POOR, {0x0} /* !! */},
  { MurmurOAAT_test,      32, 0x5363BD98, "MurmurOAAT",  "Murmur one-at-a-time", POOR,
    {0x0 /*, 0x5bd1e995*/} /* !! */ },
  { Crap8_test,           32, 0x743E97A1, "Crap8",       "Crap8", POOR, {/*0x83d2e73b, 0x97e1cc59*/} },
  { xxHash32_test,        32, 0xBA88B743, "xxHash32",    "xxHash, 32-bit for x86", POOR, {} },
  { MurmurHash1_test,     32, 0x9EA7D056, "Murmur1",     "MurmurHash1", POOR, {0xc6a4a793} /* !! */ },
  { MurmurHash2_test,     32, 0x27864C1E, "Murmur2",     "MurmurHash2 for x86, 32-bit", POOR,
    {0x10} /* !! */ },
  { MurmurHash2A_test,    32, 0x7FBD4396, "Murmur2A",    "MurmurHash2A for x86, 32-bit", POOR,
    {0x2fc301c9} /* !! */ },
#if __WORDSIZE >= 64
  { MurmurHash64A_test,   64, 0x1F0D3804, "Murmur2B",    "MurmurHash64A for x64, 64-bit", POOR,
    {0xc6a4a7935bd1e995ULL} },
#endif
#ifdef HAVE_INT64
  { MurmurHash64B_test,   64, 0xDD537C05, "Murmur2C",    "MurmurHash64B for x86, 64-bit", POOR,
    {0x10, 0xffffffff00000010 } /* !! *00000010 */ },
#endif
  { MurmurHash3_x86_32,   32, 0xB0F57EE3, "Murmur3A",    "MurmurHash3 for x86, 32-bit", POOR,
    {0xfca58b2d} /* !! */},
  { PMurHash32_test,      32, 0xB0F57EE3, "PMurHash32",  "Shane Day's portable-ized MurmurHash3 for x86, 32-bit", POOR,
    {0xfca58b2d} /* !! */ }, // 0x4b600, 0xcc9e2d51
  { MurmurHash3_x86_128, 128, 0xB3ECE62A, "Murmur3C",    "MurmurHash3 for x86, 128-bit", POOR, {0x239b961b} },
#if !defined(DEBUG) && !defined(CROSSCOMPILING) && !defined(__aarch64__)
# ifndef HAVE_ASAN
  // TODO seeded
  { PMPML_32_CPP,         32, 0xEAE2E3CC, "PMPML_32",    "PMP_Multilinear 32-bit unseeded", POOR, {} },
#  if defined(_WIN64) || defined(__x86_64__)
  { PMPML_64_CPP,         64, 0x584CC9DF, "PMPML_64",    "PMP_Multilinear 64-bit unseeded", POOR, {} },
#  endif
# endif
#endif
  { fasthash32_test,      32, 0xE9481AFC, "fasthash32",  "fast-hash 32bit", POOR, {0x880355f21e6d1965ULL} },
  { fasthash64_test,      64, 0xA16231A7, "fasthash64",  "fast-hash 64bit", POOR, {0x880355f21e6d1965ULL} },
  { CityHash32_test,      32, 0x5C28AD62, "City32",      "Google CityHash32WithSeed (old)", POOR, {0x2eb38c9f} /* !! */},
#ifdef HAVE_INT64
  { metrohash64_test,      64, 0x6FA828C9, "metrohash64",    "MetroHash64, 64-bit", POOR, {} },
  { metrohash64_1_test,    64, 0xEE88F7D2, "metrohash64_1",  "MetroHash64_1, 64-bit (legacy)", POOR, {} },
  { metrohash64_2_test,    64, 0xE1FC7C6E, "metrohash64_2",  "MetroHash64_2, 64-bit (legacy)", GOOD, {} },
  { metrohash128_test,    128, 0x4A6673E7, "metrohash128",   "MetroHash128, 128-bit", GOOD, {} },
  { metrohash128_1_test,  128, 0x20E8A1D7, "metrohash128_1", "MetroHash128_1, 128-bit (legacy)", GOOD, {} },
  { metrohash128_2_test,  128, 0x5437C684, "metrohash128_2", "MetroHash128_2, 128-bit (legacy)", GOOD, {} },
#endif
#if defined(HAVE_SSE42) && (defined(__x86_64__) ||  defined(__aarch64__)) && !defined(_MSC_VER)
  { metrohash64crc_1_test, 64, 0x29C68A50, "metrohash64crc_1", "MetroHash64crc_1 for x64 (legacy)", POOR, {} },
  { metrohash64crc_2_test, 64, 0x2C00BD9F, "metrohash64crc_2", "MetroHash64crc_2 for x64 (legacy)", POOR, {} },
  { cmetrohash64_1_optshort_test,64, 0xEE88F7D2, "cmetrohash64_1o", "cmetrohash64_1 (shorter key optimized), 64-bit for x64", POOR, {} },
  { cmetrohash64_1_test,   64, 0xEE88F7D2, "cmetrohash64_1",  "cmetrohash64_1, 64-bit for x64", POOR, {} },
  { cmetrohash64_2_test,   64, 0xE1FC7C6E, "cmetrohash64_2",  "cmetrohash64_2, 64-bit for x64", GOOD, {} },
  { metrohash128crc_1_test,128, 0x5E75144E, "metrohash128crc_1", "MetroHash128crc_1 for x64 (legacy)", GOOD, {} },
  { metrohash128crc_2_test,128, 0x1ACF3E77, "metrohash128crc_2", "MetroHash128crc_2 for x64 (legacy)", GOOD, {} },
#endif
  { CityHash64noSeed_test, 64, 0x63FC6063, "City64noSeed","Google CityHash64 without seed (default version, misses one final avalanche)", POOR, {} },
  { CityHash64_test,      64, 0x25A20825, "City64",       "Google CityHash64WithSeed (old)", POOR, {} },
#if defined(HAVE_SSE2) && defined(HAVE_AESNI) && !defined(_MSC_VER)
  { aesnihash_test,       64, 0xA68E0D42, "aesnihash",    "majek's seeded aesnihash with aesenc, 64-bit for x64", POOR,
    {0x70736575} },
#endif
#if defined(HAVE_SSE2) && defined(__x86_64__) && !defined(_WIN32)
  { falkhash_test_cxx,    64, 0x2F99B071, "falkhash",    "falkhash.asm with aesenc, 64-bit for x64", POOR, {} },
#endif
#ifdef HAVE_MEOW_HASH
  { MeowHash32_test,      32, 0x8872DE1A, "MeowHash32low","MeowHash (requires x64 AES-NI)", POOR,
    {0x920e7c64} /* !! */},
  { MeowHash64_test,      64, 0xB04AC842, "MeowHash64low","MeowHash (requires x64 AES-NI)", POOR, {0x920e7c64} },
  { MeowHash128_test,    128, 0xA0D29861, "MeowHash",     "MeowHash (requires x64 AES-NI)", POOR, {0x920e7c64} },
#endif
  { t1ha1_64le_test,      64, 0xD6836381, "t1ha1_64le",  "Fast Positive Hash (portable, aims 64-bit, little-endian)", POOR, {} },
  { t1ha1_64be_test,      64, 0x93F864DE, "t1ha1_64be",  "Fast Positive Hash (portable, aims 64-bit, big-endian)", POOR, {} },
  { t1ha0_32le_test,      64, 0x7F7D7B29, "t1ha0_32le",  "Fast Positive Hash (portable, aims 32-bit, little-endian)", POOR, {} },
  { t1ha0_32be_test,      64, 0xDA6A4061, "t1ha0_32be",  "Fast Positive Hash (portable, aims 32-bit, big-endian)", POOR, {} },
#if __WORDSIZE >= 64
# define TIFU_VERIF       0x644236D4
#else
  // broken on certain travis
# define TIFU_VERIF       0x0
#endif
  // and now the quality hash funcs, slowest first
  { tifuhash_64,          64, TIFU_VERIF, "tifuhash_64", "Tiny Floatingpoint Unique Hash with continued egyptian fractions", POOR, {} },
  { beamsplitter_64,      64, 0x1BDF358B, "beamsplitter","A possibly universal hash made with a 10x64 s-box.", POOR, {
  // first lowest 2^32 range
  0x6fd03e4,  0x8749054,  0xb17422d,  0xbf1aff7,  0x1927cb4f, 0x2228a261,
  0x28436920, 0x28d31e38, 0x2e9c3316, 0x310cb62c, 0x3431e80b, 0x348c37b8,
  0x36d5b3ee, 0x39896fb5, 0x3a155bd3, 0x3bfdbc41, 0x3d07ff95, 0x43fcb9df,
  0x48201e9d, 0x4de9e8b4, 0x5352a40e, 0x54a3aed2, 0x55f928c3, 0x58c4ae94,
  0x5f8b8dc6, 0x5fd0ddbf, 0x65e4b06f, 0x6c446186, 0x73bc5462, 0x77e96502,
  0x7b59d0e1, 0x817843c0, 0x832c6e76, 0x8339436d, 0x83d3836b, 0x87fa8286,
  0x8cb6c554, 0x9473e0b5, 0x94e9937b, 0x959db0f6, 0x9d97be34, 0xa027b813,
  0xa3ec6b18, 0xa74fd5fe, 0xaa4ff8cf, 0xaa82a095, 0xada4c9ff, 0xae750d45,
  0xb2cf1b7e, 0xb822336b, 0xb934a433, 0xb93b1e20, 0xba6ea5dd,
  // missing all other upper 32bit ranges here...
  // last upper 2^32 range
  0xffffffff027ea452, 0xffffffff04506df8, 0xffffffff059491b4, 0xffffffff0ae153aa,
  0xffffffff0c581e21, 0xffffffff178ed4db, 0xffffffff1b4554e0, 0xffffffff20ed031e,
  0xffffffff25243d7c, 0xffffffff25d3f9f9, 0xffffffff29e0c974, 0xffffffff2c72bf5b,
  0xffffffff2d6e7317, 0xffffffff2dc6e4b1, 0xffffffff2e4661be, 0xffffffff39191879,
  0xffffffff3f6a8c22, 0xffffffff4273e668, 0xffffffff42957d46, 0xffffffff49224386,
  0xffffffff4a37ba60, 0xffffffff4d204406, 0xffffffff52b1ae6e, 0xffffffff56daedb8,
  0xffffffff5788c151, 0xffffffff58c416bf, 0xffffffff58ca0121, 0xffffffff5d72b12e,
  0xffffffff5edacdac, 0xffffffff5fd907f3, 0xffffffff657a7d41, 0xffffffff678e0581,
  0xffffffff6d32feb6, 0xffffffff6e2a4160, 0xffffffff6f19f221, 0xffffffff77f2b247,
  0xffffffff793f66b7, 0xffffffff79db63ec, 0xffffffff7ab26022, 0xffffffff7d87a1be,
  0xffffffff7e7de397, 0xffffffff83e3ca0b, 0xffffffff84c532bf, 0xffffffff88327c1f,
  0xffffffff890ffd43, 0xffffffff8de7b7b1, 0xffffffff92dad0f1, 0xffffffff930db83a,
  0xffffffff9a357352, 0xffffffff9fa14291, 0xffffffffa5a1fd23, 0xffffffffa71641e2,
  0xffffffffade1bca8, 0xffffffffae57efb3, 0xffffffffb4a0ceb2, 0xffffffffb5d7aeb8,
  0xffffffffba6d105f, 0xffffffffba8b3d4b, 0xffffffffbd287832 }, /* !! and many uncomputable more */ },
  // different verif on gcc vs clang
  { floppsyhash_64,       64, 0x0,        "floppsyhash", "slow hash designed for floating point hardware", GOOD, {} },
  { chaskey_test,         64, 0x81A90131, "chaskey",     "mouha.be/chaskey/ with added seed support", GOOD, {} },
  { siphash_test,         64, 0xC58D7F9C, "SipHash",     "SipHash 2-4 - SSSE3 optimized", GOOD, {} },
  { halfsiphash_test,     32, 0xA7A05F72, "HalfSipHash", "HalfSipHash 2-4, 32bit", GOOD, {} },
  { GoodOAAT_test,        32, 0x7B14EEE5, "GoodOAAT",    "Small non-multiplicative OAAT", GOOD, {0x3b00} },
#ifdef HAVE_INT64
  { prvhash64_64mtest,    64, 0xD37C7E74, "prvhash64_64m", "prvhash64m 64bit", GOOD, {} },
  { prvhash64_64test,     64, 0xD37C7E74, "prvhash64_64",  "prvhash64 64bit", GOOD, {} },
  { prvhash64_128test,   128, 0xB447480F, "prvhash64_128", "prvhash64 128bit", GOOD, {} },
  { prvhash64s_64test,    64, 0,          "prvhash64s_64", "prvhash64s 64bit", GOOD, {} }, // seed changes
  { prvhash64s_128test,  128, 0,          "prvhash64s_128","prvhash64s 128bit", GOOD, {} }, // seed compiler-specific
#endif
  { komihash_test,        64, 0xEE0A1C4A, "komihash",      "komihash", GOOD, {} },
  // as in rust and swift:
  { siphash13_test,       64, 0x29C010BF, "SipHash13",   "SipHash 1-3 - SSSE3 optimized", GOOD, {} },
#ifndef _MSC_VER
  { tsip_test,            64, 0x8E48155B, "TSip",        "Damian Gryski's Tiny SipHash variant", GOOD, {} },
#ifdef HAVE_INT64
  { seahash_test,         64, 0xF0374078, "seahash",     "seahash (64-bit, little-endian)", GOOD, {} },
  { seahash32low,         32, 0x712F0EE8, "seahash32low","seahash - lower 32bit", GOOD, {} },
#endif /* HAVE_INT64 */
#endif /* !MSVC */
#if defined(HAVE_SSE42) && defined(__x86_64__)
  { clhash_test,          64, 0x0, "clhash",      "carry-less mult. hash -DBITMIX (64-bit for x64, SSE4.2)", GOOD,
    {0xb3816f6a2c68e530, 711} },
#endif
#ifdef HAVE_HIGHWAYHASH
  { HighwayHash64_test,   64, 0x0,        "HighwayHash64", "Google HighwayHash (portable with dylib overhead)", GOOD, {} },
#endif
#if __WORDSIZE >= 64
  { MurmurHash3_x64_128, 128, 0x6384BA69, "Murmur3F",    "MurmurHash3 for x64, 128-bit", GOOD, {0x87c37b91114253d5ULL} },
#endif
#if defined __aarch64__
 #define MUM_VERIF            0x280B2CC6
 #define MUMLOW_VERIF         0xB13E0239
#elif defined(__GNUC__) && UINT_MAX != ULONG_MAX
 #define MUM_VERIF            0x3EEAE2D4
 #define MUMLOW_VERIF         0x520263F5
#else
 #define MUM_VERIF            0xA973C6C0
 #define MUMLOW_VERIF         0x7F898826
#endif
  { mum_hash_test,        64, MUM_VERIF,  "MUM",         "github.com/vnmakarov/mum-hash", POOR,
    {0x0} /* !! and many more. too many */ },
  { mum_low_test,         32, MUMLOW_VERIF,"MUMlow",     "github.com/vnmakarov/mum-hash", GOOD,
    {0x11fb062a, 0x3ca9411b, 0x3edd9a7d, 0x41f18860, 0x691457ba} /* !! */ },
#if defined(__GNUC__) && UINT_MAX != ULONG_MAX
 #define MIR_VERIF            0x00A393C8
 #define MIRLOW_VERIF         0xE320CE68
#else
 #define MIR_VERIF            0x422A66FC
 #define MIRLOW_VERIF         0xD50D1F09
#endif
#ifdef HAVE_INT64
  // improved MUM
  { mirhash_test,         64, MIR_VERIF,    "mirhash",            "mirhash", GOOD,
    {0x0, 0x5e74c778, 0xa521f17b, 0xe0ab70e3} /* 2^36 !! (plus all hi ranges) */ },
  { mirhash32low,         32, MIRLOW_VERIF, "mirhash32low",       "mirhash - lower 32bit", POOR,
    {0x0, 0x5e74c778, 0xa521f17b, 0xe0ab70e3 } /* !! */ },
  { mirhashstrict_test,   64, 0x422A66FC,   "mirhashstrict",      "mirhashstrict (portable, 64-bit, little-endian)", GOOD,
    {0x7fcc747f} /* tested ok */ },
  { mirhashstrict32low,   32, 0xD50D1F09,   "mirhashstrict32low", "mirhashstrict - lower 32bit", POOR,
    {0x7fcc747f} /* !! */ },
#endif
  { CityHash64_low_test,  32, 0xCC5BC861, "City64low",   "Google CityHash64WithSeed (low 32-bits)", GOOD, {} },
#if defined(__SSE4_2__) && defined(__x86_64__)
  { CityHash128_test,    128, 0x6531F54E, "City128",     "Google CityHash128WithSeed (old)", GOOD, {} },
  { CityHashCrc128_test, 128, 0xD4389C97, "CityCrc128",  "Google CityHashCrc128WithSeed SSE4.2 (old)", GOOD, {} },
#endif

#if defined(__FreeBSD__)
#  define FARM64_VERIF        0x0
#  define FARM128_VERIF       0x0
#else
#  define FARM64_VERIF        0xEBC4A679
#  define FARM128_VERIF       0x305C0D9A
#endif
  { FarmHash32_test,      32, 0/*0x2E226C14*/,   "FarmHash32",  "Google FarmHash32WithSeed", GOOD, {0x2b091701} /* !! */},
  { FarmHash64_test,      64, FARM64_VERIF, "FarmHash64",  "Google FarmHash64WithSeed", GOOD, {} },
 //{ FarmHash64noSeed_test,64, 0xA5B9146C,  "Farm64noSeed","Google FarmHash64 without seed (default, misses on final avalanche)", POOR, {} },
  { FarmHash128_test,    128, FARM128_VERIF,"FarmHash128", "Google FarmHash128WithSeed", GOOD, {} },
#if defined(__SSE4_2__) && defined(__x86_64__)
  { farmhash32_c_test,    32, 0/*0xA2E45238*/,   "farmhash32_c", "farmhash32_with_seed (C99)", GOOD,
    {0x2b091701} /* !! */},
  { farmhash64_c_test,    64, FARM64_VERIF, "farmhash64_c",  "farmhash64_with_seed (C99)", GOOD, {} },
  { farmhash128_c_test,  128, FARM128_VERIF,"farmhash128_c", "farmhash128_with_seed (C99)", GOOD, {} },
#endif

  { xxHash64_test,        64, 0x024B7CF4, "xxHash64",    "xxHash, 64-bit", GOOD, {} },
#if 0
  { xxhash256_test,       64, 0x024B7CF4, "xxhash256",   "xxhash256, 64-bit unportable", GOOD, {} },
#endif
  { SpookyHash32_test,    32, 0x3F798BBB, "Spooky32",    "Bob Jenkins' SpookyHash, 32-bit result", GOOD,
    { 0x111af082, 0x26bb3cda, 0x94c4f96c, 0xec24c166 } /* !! */},
  { SpookyHash64_test,    64, 0xA7F955F1, "Spooky64",    "Bob Jenkins' SpookyHash, 64-bit result", GOOD, {} },
  { SpookyHash128_test,  128, 0x8D263080, "Spooky128",   "Bob Jenkins' SpookyHash, 128-bit result", GOOD, {} },
  { SpookyV2_32_test,     32, 0xA48BE265, "SpookyV2_32",  "Bob Jenkins' SpookyV2, 32-bit result", GOOD, {} },
  { SpookyV2_64_test,     64, 0x972C4BDC, "SpookyV2_64",  "Bob Jenkins' SpookyV2, 64-bit result", GOOD, {} },
  { SpookyV2_128_test,   128, 0x893CFCBE, "SpookyV2_128", "Bob Jenkins' SpookyV2, 128-bit result", GOOD, {} },
  { pengyhash_test,       64, 0x1FC2217B, "pengyhash",   "pengyhash", GOOD, {} },
  { mx3rev1hash64_test,   64, 0x4DB51E5B, "mx3-rev1",    "mx3 revision 1 64bit", GOOD, {0x10} /* !! and all & 0x10 */},
  { mx3hash64_test,       64, 0x527399AD, "mx3",         "mx3 revision 2 64bit", GOOD, {} },
#if defined(HAVE_SSE42) &&  (defined(__x86_64__) ||  defined(__aarch64__)) && !defined(_MSC_VER)
  { umash32,              32, 0x03E16CA1, "umash32",     "umash 32", GOOD, {0x90e37057} /* !! */},
  { umash32_hi,           32, 0xE29D613C, "umash32_hi",  "umash 32 hi", GOOD, {} },
  { umash,                64, 0x4542288C, "umash64",     "umash 64", GOOD, {} },
  { umash128,            128, 0xDA4E82B6, "umash128",    "umash 128", GOOD, {} },
#endif
  { halftime_hash_style64_test,  64, 0x0, "halftime_hash64",    "NH tree hash variant", GOOD,
    {0xc61d672b, 0xcc70c4c1798e4a6f, 0xd3833e804f4c574b, 0xecfc1357d65941ae, 0xbe1927f97b8c43f1, 
     0xf4d4beb14ae042bbULL, 0x9a9b4c4e44dd48d1ULL} }, // not vulnerable
  { halftime_hash_style128_test, 64, 0x0, "halftime_hash128",   "NH tree hash variant", GOOD,
    {0xc61d672b, 0xcc70c4c1798e4a6f, 0xd3833e804f4c574b, 0xecfc1357d65941ae, 0xbe1927f97b8c43f1, 
     0xf4d4beb14ae042bbULL, 0x9a9b4c4e44dd48d1ULL} },
  { halftime_hash_style256_test, 64, 0x0, "halftime_hash256",   "NH tree hash variant", GOOD,
    {0xc61d672b, 0xcc70c4c1798e4a6f, 0xd3833e804f4c574b, 0xecfc1357d65941ae, 0xbe1927f97b8c43f1, 
     0xf4d4beb14ae042bbULL, 0x9a9b4c4e44dd48d1ULL} },
  { halftime_hash_style512_test, 64, 0x0, "halftime_hash512",   "NH tree hash variant", GOOD,
    {0xc61d672b, 0xcc70c4c1798e4a6f, 0xd3833e804f4c574b, 0xecfc1357d65941ae, 0xbe1927f97b8c43f1, 
     0xf4d4beb14ae042bbULL, 0x9a9b4c4e44dd48d1ULL} },

  { t1ha2_atonce_test,           64, 0x8F16C948, "t1ha2_atonce",    "Fast Positive Hash (portable", GOOD, {
    } },
  { t1ha2_stream_test,           64, 0xDED9B580, "t1ha2_stream",    "Fast Positive Hash (portable)", POOR, {} },
  { t1ha2_atonce128_test,       128, 0xB44C43A1, "t1ha2_atonce128", "Fast Positive Hash (portable)", GOOD, {} },
  { t1ha2_stream128_test,       128, 0xE929E756, "t1ha2_stream128", "Fast Positive Hash (portable)", POOR, {} },
#if T1HA0_AESNI_AVAILABLE
#  ifndef _MSC_VER
  { t1ha0_ia32aes_noavx_test,  64, 0xF07C4DA5, "t1ha0_aes_noavx", "Fast Positive Hash (AES-NI)", GOOD, {} },
#  endif
#  if defined(__AVX__)
  { t1ha0_ia32aes_avx1_test,   64, 0xF07C4DA5, "t1ha0_aes_avx1",  "Fast Positive Hash (AES-NI & AVX)", GOOD, {} },
#  endif /* __AVX__ */
#  if defined(__AVX2__)
  { t1ha0_ia32aes_avx2_test,   64, 0x8B38C599, "t1ha0_aes_avx2",  "Fast Positive Hash (AES-NI & AVX2)", GOOD, {} },
#  endif /* __AVX2__ */
#endif /* T1HA0_AESNI_AVAILABLE */
#ifdef HAVE_AHASH_C
  // aHash does not adhere to a fixed output
  { ahash64_test,         64, 0x00000000, "ahash64",     "ahash 64bit", GOOD, {} },
#endif
  { xxh3_test,            64, 0x39CD9E4A, "xxh3",        "xxHash v3, 64-bit", GOOD, // no known bad seeds
    {0x47ebda34,             // 32bit bad seed
     /* 0xbe4ba423396cfeb8,  // kSecret
     0x396cfeb8, 0xbe4ba423, // kSecret
     0x6782737bea4239b9,     // bitflip1 ^ input
     0xaf56bc3b0996523a,     // bitflip2 ^ input[last 8]
     */
    }},
  { xxh3low_test,         32, 0xFAE8467B, "xxh3low",     "xxHash v3, 64-bit, low 32-bits part", GOOD,
    {0x47ebda34} /* !! */},
  { xxh128_test,         128, 0xEB61B3A0, "xxh128",      "xxHash v3, 128-bit", GOOD,
    {0x47ebda34}},
  { xxh128low_test,       64, 0x54D1CC70, "xxh128low",   "xxHash v3, 128-bit, low 64-bits part", GOOD,
    {0x47ebda34}},
#ifdef HAVE_BIT32
  { wyhash32_test,        32, 0x09DE8066, "wyhash32",       "wyhash v3 (32-bit native)", GOOD,
    { 0x429dacdd, 0xd637dbf3 } /* !! */ },
#else
  { wyhash32low,          32, 0x7DB3559D, "wyhash32low",    "wyhash v3 lower 32bit", GOOD,
    { 0x429dacdd, 0xd637dbf3 } /* !! */ },
#endif
#ifdef HAVE_INT64
  { wyhash_test,          64, 0x67031D43, "wyhash",         "wyhash v3 (64-bit)", GOOD,
    // all seeds with those lower bits
    { 0x14cc886e, 0x1bf4ed84, 0x14cc886e14cc886eULL} /* !! 2^33 bad seeds, but easy to check */ },
  //{ wyhash_condom_test,   64, 0x7C62138D, "wyhash_condom",  "wyhash v3 condom 2 (64-bit)", GOOD, { } },
#endif
  { nmhash32_test,        32, 0x12A30553, "nmhash32",       "nmhash32", GOOD, {}},
  { nmhash32x_test,       32, 0xA8580227, "nmhash32x",      "nmhash32x", GOOD, {}},
};

size_t numLegacyHashes(void) {
    return sizeof(g_hashes) / sizeof(LegacyHashInfo);
}

LegacyHashInfo * numLegacyHash(size_t num) {
    if (num >= numLegacyHashes()) {
        return NULL;
    }
    return &g_hashes[num];
}

LegacyHashInfo * findLegacyHash ( const char * name )
{
  for(size_t i = 0; i < sizeof(g_hashes) / sizeof(LegacyHashInfo); i++)
  {
    if(_stricmp(name,g_hashes[i].name) == 0)
      return &g_hashes[i];
  }

  return NULL;
}

// optional hash state initializers
void Hash_init (LegacyHashInfo* info) {
  if (info->hash == sha2_224_64)
    sha224_init(&ltc_state);
  else if (info->hash == rmd128)
    rmd128_init(&ltc_state);
  else if(info->hash == tabulation_32_test)
    tabulation_32_init();
#ifdef __SIZEOF_INT128__
  else if(info->hash == multiply_shift ||
          info->hash == pair_multiply_shift)
    multiply_shift_init();
  else if(info->hash == poly_1_mersenne ||
          info->hash == poly_2_mersenne ||
          info->hash == poly_3_mersenne ||
          info->hash == poly_4_mersenne)
    poly_mersenne_init();
  else if(info->hash == tabulation_test)
    tabulation_init();
#endif
#if defined(HAVE_SSE42) && defined(__x86_64__)
  else if(info->hash == clhash_test)
    clhash_init();
  //else if(info->hash == umash32_test ||
  //        info->hash == umash32hi_test ||
  //        info->hash == umash64_test ||
  //        info->hash == umash128_test)
  //  umash_init();
#endif
  else if (info->hash == VHASH_32 || info->hash == VHASH_64)
    VHASH_init();
#ifdef HAVE_HIGHWAYHASH
  else if(info->hash == HighwayHash64_test)
    HighwayHash_init();
#endif
#ifndef _MSC_VER
  else if(info->hash == tsip_test)
    tsip_init();
#endif
  else if(info->hash == chaskey_test)
    chaskey_init();
  else if (info->hash == halftime_hash_style64_test ||
           info->hash == halftime_hash_style128_test ||
           info->hash == halftime_hash_style256_test ||
           info->hash == halftime_hash_style512_test)
    halftime_hash_init();
}

// Needed for hashed with a few bad seeds, to reject this seed and generate a new one.
// (GH #99)
void Bad_Seed_init (pfHash hash, uint32_t &seed) {
  if(hash ==
#ifdef HAVE_BIT32
          wyhash32_test
#else
          wyhash32low
#endif
          )
    wyhash32_seed_init(seed);
  // zero-seed hashes:
  else if (!seed && (hash == fletcher2_test ||
                     hash == fletcher4_test || hash == Bernstein_test || hash == sdbm_test ||
                     hash == JenkinsOOAT_test || hash == JenkinsOOAT_perl_test ||
                     hash == SuperFastHash_test || hash == MurmurOAAT_test ||
                     hash == o1hash_test))
    seed++;
  else if (hash == Crap8_test && (seed == 0x83d2e73b || seed == 0x97e1cc59))
    seed++;
  else if (hash == MurmurHash1_test && seed == 0xc6a4a793)
    seed++;
  else if (hash == MurmurHash2_test && seed == 0x10)
    seed++;
  else if (hash == MurmurHash2A_test && seed == 0x2fc301c9)
    seed++;
  else if((hash == MurmurHash3_x86_32 || hash == PMurHash32_test) && seed == 0xfca58b2d)
    seed++;
  else if (hash == MurmurHash3_x86_128 && seed == 0x239b961b)
    seed++;
#ifdef HAVE_INT64
  else if(hash == wyhash_test) {
    size_t seedl = seed;
    wyhash_seed_init(seedl);
    seed = seedl;
  }
  else if(hash == mirhash_test)
    mirhash_seed_init(seed);
  else if(hash == mirhash32low)
    mirhash32_seed_init(seed);
  else if(hash == mirhashstrict32low && seed == 0x7fcc747f)
    seed++;
  else if(hash == MurmurHash64B_test)
    MurmurHash64B_seed_init(seed);
#endif
#ifdef __SIZEOF_INT128__
  else if(hash == multiply_shift)
    multiply_shift_seed_init(seed);
  else if((hash == poly_2_mersenne && seed == 0x60e8512c) ||
          (hash == poly_3_mersenne && seed == 0x3d25f745))
    seed++;
#endif
#if defined(__SSE4_2__) && defined(__x86_64__)
  else if (hash == clhash_test && seed == 0x0)
    seed++;
#endif
}

// Optional hash seed initializer, for expensive seeding.
bool Hash_Seed_init (pfHash hash, size_t seed) {
  addVCodeInput(seed);

  uint32_t seed32 = seed;

  //if (hash == VHASH_32 || hash == VHASH_64)
  //  VHASH_seed_init(seed);
  if(hash == tabulation_32_test)
    tabulation_32_seed_init(seed);
#ifdef __SIZEOF_INT128__
  else if(hash == multiply_shift || hash == pair_multiply_shift)
    multiply_shift_seed_init(seed32);
  else if(/*hash == poly_0_mersenne || */
          hash == poly_1_mersenne ||
          hash == poly_2_mersenne ||
          hash == poly_3_mersenne ||
          hash == poly_4_mersenne)
    poly_mersenne_seed_init(seed32);
  else if(hash == tabulation_test)
    tabulation_seed_init(seed);
#endif
#if defined(HAVE_SSE42) && defined(__x86_64__)
  else if (hash == clhash_test)
    clhash_seed_init(seed);
# ifndef _MSC_VER  
  else if (hash == umash32 ||
          hash == umash32_hi ||
          hash == umash ||
          hash == umash128)
    umash_seed_init(seed);
# endif
  else if (hash == halftime_hash_style64_test || hash == halftime_hash_style128_test ||
           hash == halftime_hash_style256_test || hash == halftime_hash_style512_test)
    halftime_hash_seed_init(seed);
  /*
  else if(hash == hashx_test)
    hashx_seed_init(info, seed);
  */
#endif
  else
      return false;
  return true;
}

//-----------------------------------------------------------------------------
bool hash_is_very_slow(pfHash hash) {
    // known very slow hashes (typically > 500 cycle/hash)
    const struct { pfHash h; } slowhashes[] = {
        { sha2_224                 },
        { sha2_224_64              },
        { sha2_256                 },
        { sha2_256_64              },
        { rmd128                   },
        { rmd160                   },
        { rmd256                   },
        { blake2s128_test          },
        { blake2s160_test          },
        { blake2s224_test          },
        { blake2s256_test          },
        { blake2s256_64            },
        { blake2b160_test          },
        { blake2b224_test          },
        { blake2b256_test          },
        { blake2b256_64            },
        { sha3_256                 },
        { sha3_256_64              },
        { tifuhash_64              },
        { floppsyhash_64           },
        { beamsplitter_64          },
    };

    for (int i=0; i<sizeof(slowhashes)/sizeof(slowhashes[0]); i++) {
        if (slowhashes[i].h == hash) {
            return true;
        }
    }

    return false;
}

bool hash_is_slow(pfHash hash) {
    if (hash_is_very_slow(hash)) {
        return true;
    }

    // known somewhat slow hashes
    const struct { pfHash h; } slowhashes[] = {
        { o1hash_test                 },
        { halftime_hash_style64_test  },
        { halftime_hash_style128_test },
        { halftime_hash_style256_test },
        { halftime_hash_style512_test },
    };

    for (int i=0; i<sizeof(slowhashes)/sizeof(slowhashes[0]); i++) {
        if (slowhashes[i].h == hash) {
            return true;
        }
    }

    return false;
}
