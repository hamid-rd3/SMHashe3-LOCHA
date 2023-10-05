/*
 * ###YOURHASHNAME
 * Copyright (C) 2022 ###YOURNAME
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#include "Platform.h"
#include "Hashlib.h"
#include <bits/stdc++.h>
#include <functional>
#include <iostream>
#include <algorithm>
// shuffled prime numbers lower than 1000
static uint16_t const S_table1[97]{
    421, 311, 977, 331, 503, 139, 683, 907, 73, 151, 797, 83, 761, 373, 157, 71, 127, 601, 607, 881, 967, 569, 281, 353, 659, 839, 641, 41, 401, 587, 113, 13, 293, 769, 103, 809, 443, 29, 53, 599, 829, 277, 557, 523, 647, 953, 433, 457, 307, 857, 347, 193, 181, 757, 823, 383, 673, 461, 487, 137, 223, 439, 677, 109, 739, 449, 719, 701, 409, 491, 877, 971, 59, 131, 941, 349, 389, 547, 859, 197, 101, 431, 149, 367, 499, 821, 17, 251, 929, 227, 11, 379, 271, 743, 89, 241};

// random shuffled prime numbers between Range of the paper(probably)
static uint16_t const S_table2[67]{
    2281, 2239, 2309, 2083, 2459, 2081, 2069, 2441, 2153, 2347, 2287, 2203, 2179, 2027, 2293, 2383, 2251, 2521, 2243, 2039, 2129, 2531, 2339, 2131, 2437, 2089, 2011, 2473, 2273, 2003, 2341, 2267, 2377, 2113, 2237, 2423, 2417, 2143, 2269, 2087, 2099, 2017, 2447, 2141, 2371, 2311, 2137, 2111, 2477, 2297, 2503, 2351, 2207, 2411, 2357, 2161, 2467, 2029, 2053, 2381, 2399, 2221, 2389, 2213, 2063, 2333, 2393};

static uint16_t S_table3[256]{
    1579, 337, 1009, 353, 541, 163, 1607, 1367, 97, 173, 1123, 1231, 797, 397, 179, 89, 149, 619, 1223, 911, 991, 1553, 311, 1373, 683, 863, 1307, 59, 431, 607, 1429, 1543, 1303, 809, 127, 827, 1423, 1489, 1109, 617, 859, 1619, 1103, 563, 673, 1657, 457, 479, 331, 1499, 367, 223, 1493, 1229, 853, 409, 1117, 487, 1289, 157, 239, 1637, 709, 1061, 1129, 467, 1567, 733, 433, 521, 907, 997, 73, 151, 1627, 373, 419, 571, 883, 1201, 113, 449, 1601, 389, 523, 839, 31, 271, 953, 241, 23, 401, 1531, 769, 1279, 1153, 257, 131, 233, 439, 193, 41, 1597, 1213, 421, 229, 661, 1259, 61, 1409, 19, 1193, 1049, 281, 1019, 1381, 821, 613, 181, 1487, 1319, 503, 967, 499, 1447, 17, 1063, 1051, 349, 811, 727, 1321, 1249, 977, 877, 11, 277, 1181, 67, 887, 283, 13, 643, 1471, 739, 109, 1031, 587, 101, 929, 829, 47, 359, 773, 677, 599, 941, 491, 53, 1093, 1091, 1187, 569, 1021, 1217, 601, 1433, 37, 857, 1523, 1327, 641, 751, 1609, 1151, 383, 1087, 1571, 1613, 823, 1583, 71, 701, 1097, 1163, 83, 269, 761, 647, 211, 1301, 1171, 227, 653, 1483, 1439, 787, 103, 167, 757, 1039, 1283, 1453, 1277, 509, 577, 463, 263, 317, 659, 347, 1427, 557, 1399, 937, 379, 547, 631, 691, 1297, 313, 139, 1069, 1361, 1559, 79, 107, 983, 919, 593, 191, 947, 43, 199, 881, 1511, 197, 1621, 29, 307, 1481, 1033, 743, 1451, 443, 1291, 1013, 1237, 719, 137, 251, 1549, 293, 971, 461, 1459};
static const unsigned char locha_padding[64] =
    {
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

typedef struct
{
    uint8_t state[24]{0};
    size_t count[2]{0};
    uint8_t buffer[64]{0};
} LOCHA1_CTX;
typedef struct
{
    uint8_t hexdigit[24]{0};
    uint32_t fstep_conv{1};
    uint8_t sstep_conv[3]{0, 0, 7};
} Initials;

static void LOCHA_FirstStep(const uint8_t *buffer, uint8_t *pk, uint32_t &fstep_conv)
{
    for (uint8_t j{0}; j < 8; j++)
    {
        // 32 non_printable ASCIIcode
        if ((buffer[j] >= 31) && (buffer[j] <= 127))
            pk[j] = (buffer[j] - 31);
        else
            pk[j] = 1;
        fstep_conv *= S_table1[pk[j]];
    }
}

static void LOCHA_FirstStep_new(const uint8_t *buffer, uint8_t *pk, uint32_t &fstep_conv)
{
    for (uint8_t j{0}; j < 8; j++)
    {
        // 32 non_printable ASCIIcode
        memcpy(&pk[j], (buffer + j), 1);
        // std::cout << (size_t)(*fstep_conv) << "*" << S_table3[pk[j]] << "=";
        fstep_conv = static_cast<uint64_t>((fstep_conv) * S_table3[pk[j]]);
        fstep_conv = ((fstep_conv << j * 4) | (fstep_conv >> (32 - j * 4)));

        // std::cout << (size_t)(*fstep_conv) << std::endl;
    }
}

template <bool newver>
static void LOCHA_Steps(LOCHA1_CTX *context, uint8_t *tempout, Initials *initial)
{
    for (uint8_t i{0}; i < 8; i++)
    {
        uint8_t pk[8];
        if (newver)
            LOCHA_FirstStep_new(&(context->buffer[8 * i]), pk, initial->fstep_conv);
        else
            LOCHA_FirstStep(&(context->buffer[8 * i]), pk, initial->fstep_conv);
        initial->sstep_conv[0] = initial->fstep_conv % 67;
        // sub-block [1][7] ==0
        if ((S_table3[pk[1]] & 1) == 0)
            initial->sstep_conv[1] = initial->sstep_conv[0];
        else
            initial->sstep_conv[1] = 67 - initial->sstep_conv[0];
        initial->sstep_conv[2] += ((initial->sstep_conv[0] + initial->fstep_conv) % 256);
        // prevent floating point error
        if (initial->sstep_conv[2] == 0)
            initial->sstep_conv[2] += 1;
        // merge after step conversions
        uint64_t after_3conv = (((initial->fstep_conv % initial->sstep_conv[2]) + initial->fstep_conv + S_table2[initial->sstep_conv[1]]) % 255) + pk[2] + (uint8_t)((initial->hexdigit[3 * i + 1] << 4) | (initial->hexdigit[3 * i])) + (pk[0] % 127);
        // convert after_3conversion step to hexnumber

        for (uint8_t k{0}; k < 3; k++)
        {
            initial->hexdigit[3 * i + k] = ((after_3conv >> (4 * k)) & 15);
        }

        std::reverse(&initial->hexdigit[3 * i], &initial->hexdigit[3 * i + 3]);
        // std::swap(initial->hexdigit[3 * i], initial->hexdigit[3 * i+2]);
        // little endian
        // std::cout << "hex digit is "<< std::endl;
        // for (auto i :initial->hexdigit)
        //     std::cout <<( size_t )i << ",";
        // std::cout << std::endl;
        memcpy(context->state + 3 * i, initial->hexdigit + 3 * i, 3);
        std::reverse(initial->hexdigit, initial->hexdigit + 3 * (i + 1));
    }
    std::reverse(context->state, context->state + 6);
    std::transform(context->state, context->state + 24, tempout, tempout, std::plus<uint8_t>());
    std::fill_n(context->state, 24, 0);
    context->count[0] -= 64;
    context->count[1] += 64;
}

template <bool newver>
static void LOCHA1_Update(LOCHA1_CTX *context, const uint8_t *data, const size_t &len, void *out, Initials &initial)
{
    context->count[0] = len;
    if (context->count[0] == 0)
        return;
    uint8_t temp_output[24]{0};
    while (context->count[0] >= 64)
    {

        memcpy(&(context->buffer[0]), (data + context->count[1]), 64);
        LOCHA_Steps<newver>(context, temp_output, &initial);
    }

    memcpy(&context->buffer[0], (data + context->count[1]), context->count[0]);
    memcpy(context->buffer + context->count[0], locha_padding,
           (64 - context->count[0]));
    context->count[0] += (64 - context->count[0]);
    LOCHA_Steps<newver>(context, temp_output, &initial);
    // std::cout << "tempout " << std::endl;

    // for (size_t i{0}; i < 24; i++)
    // {
    //     std::cout << (size_t)temp_output[i] << ",";
    // }
    // std::cout << "tempout is end " << std::endl;

    // probably num of input bits is divisible by 64
    try
    {
        if (context->count[0])
            throw(context->count[0]);
        else
            // remove 4end-pointless zeros from hexdigits,(4 bit not 8!)
            for (uint8_t i{0}; i < 24; i += 2)
            {
                *((uint8_t *)(out) + i / 2) += (((temp_output[i] & 0xF) << 4) | (temp_output[i + 1] & 0xF));
            }
    }
    catch (const size_t &reminded_bits)
    {
        std::cout << "padding is done wrong ! " << reminded_bits << "bits remidned" << std::endl;
    }
}
inline void LOCHA1_Seed(Initials *initial, const uint64_t &_seed)
{

    // std::shuffle(S_table3,S_table3+24,std::default_random_engine(_seed));
    initial->fstep_conv += (_seed);
    // std::cout<<initial->fstep_conv<<",";
    initial->fstep_conv ^= (_seed >> 16);
    // std::cout << initial->fstep_conv << ",";
    initial->fstep_conv += (_seed >> 32);
    // std::cout << initial->fstep_conv << ",";
    initial->fstep_conv ^= (_seed >> 48);

    if (initial->fstep_conv == 0)
        initial->fstep_conv += 2;

    // std::cout << "|fstep after : " << (size_t)initial->fstep_conv << std::endl;
}
//------------------------------------------------------------
template <bool newver>
static void LOCHA1(const void *in, const size_t len, const size_t seed, void *out)
{
    LOCHA1_CTX locha_ctx;
    uint8_t *hash[12]{0};
    Initials initial_var;
    LOCHA1_Seed(&initial_var, seed);
    // std::cout <<"hex is : "<<(size_t)initial_var.hexdigit[1]<<std::endl;
    LOCHA1_Update<newver>(&locha_ctx, (const uint8_t *)in, len, hash, initial_var);
    memcpy((uint8_t *)out, hash, 12);
    // std::cout << "input is "
    //           << ":";
    // for (size_t i{0}; i < len; i += 1)
    // {
    //     std::cout << (size_t)input[i] << ",";
    // }
    // std::cout << "output is "
    //           << ":";
    // for (uint8_t i{0}; i < 12; i += 1)
    // {
    //     std::cout << (size_t)hash[i] << ",";
    // }
    // std::cout << std::endl;
}

//------------------------------------------------------------
REGISTER_FAMILY(locha,
                $.src_url = "https://doi.org/10.1016/j.procs.2014.05.453",
                $.src_status = HashFamilyInfo::SRC_FROZEN);

REGISTER_HASH(LOCHA_1,
              $.desc = "Light-weight One-way Cryptographic Hash Algorithm for Wireless Sensor Network",
              $.hash_flags =
                  FLAG_IMPL_CANONICAL_LE |
                  FLAG_IMPL_ROTATE |
                  FLAG_IMPL_INCREMENTAL |
                  FLAG_IMPL_VERY_SLOW,
              $.impl_flags =
                  FLAG_HASH_CRYPTOGRAPHIC_WEAK |
                  FLAG_HASH_ENDIAN_INDEPENDENT |
                  FLAG_HASH_NO_SEED,
              $.bits = 96,
              $.hashfn_native = LOCHA1<false>,
              $.hashfn_bswap = LOCHA1<false>);

REGISTER_HASH(LOCHA_2,
              $.desc = "Light-weight One-way Cryptographic Hash Algorithm for Wireless Sensor Network-vesion2",
              $.hash_flags =
                  FLAG_IMPL_CANONICAL_LE |
                  FLAG_IMPL_ROTATE |
                  FLAG_IMPL_INCREMENTAL |
                  FLAG_IMPL_VERY_SLOW,
              $.impl_flags =
                  FLAG_HASH_CRYPTOGRAPHIC_WEAK |
                  FLAG_HASH_ENDIAN_INDEPENDENT |
                  FLAG_HASH_NO_SEED,
              $.bits = 96,
              $.hashfn_native = LOCHA1<true>,
              $.hashfn_bswap = LOCHA1<true>);