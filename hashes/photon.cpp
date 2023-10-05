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
// #include<iostream>
// #include <string.h>
// #include <math.h>

#define ROUND 12
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

#define S 4
#define D 5
#define RATE 20
#define RATEP 16
#define DIGESTSIZE 80

typedef uint8_t byte;
typedef uint32_t u32;
typedef uint64_t u64;

typedef uint32_t CWord;
typedef u32 tword;

typedef struct
{
    u64 h;
    u64 l;
} u128; // state word

#define S 4
const byte ReductionPoly = 0x3;
const byte WORDFILTER = ((byte)1 << S) - 1;

 

/* to be completed for one time pass mode */
unsigned long long MessBitLen = 0;

static byte RC[D][12] = {

    {1, 3, 7, 14, 13, 11, 6, 12, 9, 2, 5, 10},
    {0, 2, 6, 15, 12, 10, 7, 13, 8, 3, 4, 11},
    {2, 0, 4, 13, 14, 8, 5, 15, 10, 1, 6, 9},
    {7, 5, 1, 8, 11, 13, 0, 10, 15, 4, 3, 12},
    {5, 7, 3, 10, 9, 15, 2, 8, 13, 6, 1, 14}

};

static const byte MixColMatrix[D][D] = {

    {1, 2, 9, 9, 2},
    {2, 5, 3, 8, 13},
    {13, 11, 10, 12, 1},
    {1, 15, 2, 3, 14},
    {14, 14, 8, 5, 12}

};

static byte sbox[16] = {12, 5, 6, 11, 9, 0, 10, 13, 3, 14, 15, 8, 4, 7, 1, 2};

static FORCE_INLINE byte FieldMult(byte a, byte b)
{
    byte x = a, ret = 0;
    int i;
    for (i = 0; i < S; i++)
    {
        if ((b >> i) & 1)
            ret ^= x;
        if ((x >> (S - 1)) & 1)
        {
            x <<= 1;
            x ^= ReductionPoly;
        }
        else
            x <<= 1;
    }
    return ret & WORDFILTER;
}

static FORCE_INLINE void PrintState(byte state[D][D])
{
    if (!0)
        return;
    int i, j;
    for (i = 0; i < D; i++)
    {
        for (j = 0; j < D; j++)
            printf("%2X ", state[i][j]);
        printf("\n");
    }
    printf("\n");
}

static FORCE_INLINE void PrintState_Column(CWord state[D])
{
    if (!0)
        return;
    int i, j;
    for (i = 0; i < D; i++)
    {
        for (j = 0; j < D; j++)
            printf("%2X ", (state[j] >> (i * S)) & WORDFILTER);
        printf("\n");
    }
    printf("\n");
}

static FORCE_INLINE void printDigest(const byte *digest)
{
    int i;
    for (i = 0; i < DIGESTSIZE / 8; i++)
        printf("%.2x", digest[i]);
    printf("\n");
}

static FORCE_INLINE void AddKey(byte state[D][D], int round)
{
    int i;
    for (i = 0; i < D; i++)
        state[i][0] ^= RC[i][round];
}

static FORCE_INLINE void SubCell(byte state[D][D])
{
    int i, j;
    for (i = 0; i < D; i++)
        for (j = 0; j < D; j++)
            state[i][j] = sbox[state[i][j]];
}

static FORCE_INLINE void ShiftRow(byte state[D][D])
{
    int i, j;
    byte tmp[D];
    for (i = 1; i < D; i++)
    {
        for (j = 0; j < D; j++)
            tmp[j] = state[i][j];
        for (j = 0; j < D; j++)
            state[i][j] = tmp[(j + i) % D];
    }
}

static FORCE_INLINE void MixColumn(byte state[D][D])
{
    int i, j, k;
    byte tmp[D];
    for (j = 0; j < D; j++)
    {
        for (i = 0; i < D; i++)
        {
            byte sum = 0;
            for (k = 0; k < D; k++)
                sum ^= FieldMult(MixColMatrix[i][k], state[k][j]);
            tmp[i] = sum;
        }
        for (i = 0; i < D; i++)
            state[i][j] = tmp[i];
    }
}

static FORCE_INLINE void Permutation(byte state[D][D], int R)
{
    int i;
    for (i = 0; i < R; i++)
    {
        if (0)
            printf("--- Round %d ---\n", i);
        AddKey(state, i);
        PrintState(state);
#ifndef _TABLE_
        SubCell(state);
        PrintState(state);
        ShiftRow(state);
        PrintState(state);
        MixColumn(state);
#else
        SCShRMCS(state);
#endif
        PrintState(state);
    }
}

/* get NoOfBits bits values from str starting from BitOffSet-th bit
 * Requirement: NoOfBits <= 8 */
byte GetByte(const byte *str, int BitOffSet, int NoOfBits)
{

    return (str[BitOffSet >> 3] >> (4 - (BitOffSet & 0x4))) & WORDFILTER;
}

static FORCE_INLINE void WordXorByte(byte state[D][D], const byte *str, int BitOffSet, int WordOffSet, int NoOfBits)
{
    int i = 0;
    while (i < NoOfBits)
    {
        state[(WordOffSet + (i / S)) / D][(WordOffSet + (i / S)) % D] ^= GetByte(str, BitOffSet + i, min(S, NoOfBits - i)) << (S - min(S, NoOfBits - i));
        i += S;
    }
}

/* ensure NoOfBits <=8 */
static FORCE_INLINE void WriteByte(byte *str, byte value, int BitOffSet, int NoOfBits)
{
    int ByteIndex = BitOffSet >> 3;
    int BitIndex = BitOffSet & 0x7;
    byte localFilter = (((byte)1) << NoOfBits) - 1;
    value &= localFilter;
    if (BitIndex + NoOfBits <= 8)
    {
        str[ByteIndex] &= ~(localFilter << (8 - BitIndex - NoOfBits));
        str[ByteIndex] |= value << (8 - BitIndex - NoOfBits);
    }
    else
    {
        u32 tmp = ((((u32)str[ByteIndex]) << 8) & 0xFF00) | (((u32)str[ByteIndex + 1]) & 0xFF);
        tmp &= ~((((u32)localFilter) & 0xFF) << (16 - BitIndex - NoOfBits));
        tmp |= (((u32)(value)) & 0xFF) << (16 - BitIndex - NoOfBits);
        str[ByteIndex] = (tmp >> 8) & 0xFF;
        str[ByteIndex + 1] = tmp & 0xFF;
    }
}

static FORCE_INLINE void WordToByte(byte state[D][D], byte *str, int BitOffSet, int NoOfBits)
{
    int i = 0;
    while (i < NoOfBits)
    {
        WriteByte(str, (state[i / (S * D)][(i / S) % D] & WORDFILTER) >> (S - min(S, NoOfBits - i)), BitOffSet + i, min(S, NoOfBits - i));
        i += S;
    }
}

static FORCE_INLINE void PermutationOnByte(byte *in, int R)
{
    byte state[D][D];
    int i;
    for (i = 0; i < D * D; i++)
        state[i / D][i % D] = GetByte(in, i * S, S);
    Permutation(state, R);
    WordToByte(state, in, 0, D * D * S);
}

static FORCE_INLINE void Init(byte state[D][D])
{

    int i, j;
    MessBitLen = 0;

    for (i = 0; i < D; i++)
        for (j = 0; j < D; j++)
            state[i][j] = 0;
    byte presets[3];
    presets[0] = (DIGESTSIZE >> 2) & 0xFF;
    presets[1] = RATE & 0xFF;
    presets[2] = RATEP & 0xFF;
    WordXorByte(state, presets, 0, D * D - 24 / S, 24);
}

static FORCE_INLINE void CompressFunction(byte state[D][D], const byte *mess, int BitOffSet)
{
    WordXorByte(state, mess, BitOffSet, 0, RATE);
    Permutation(state, ROUND);
}

/* assume DIGESTSIZE is multiple of RATEP, RATEP is multiple of S */
static FORCE_INLINE void Squeeze(byte state[D][D], byte *digest)
{
    int i = 0;
    while (1)
    {
        WordToByte(state, digest, i, min(RATEP, DIGESTSIZE - i));
        i += RATEP;
        if (i >= DIGESTSIZE)
            break;
        Permutation(state, ROUND);
    }
}

static FORCE_INLINE void hash(byte *digest, const byte *mess, int BitLen)
{
    byte state[D][D], padded[(int)(RATE / 8.0) + 1];
    Init(state);
    int MessIndex = 0;
    while (MessIndex <= (BitLen - RATE))
    {
        CompressFunction(state, mess, MessIndex);
        MessIndex += RATE;
    }
    int i, j;
    for (i = 0; i < ((int)(RATE / 8.0) + 1); i++)
        padded[i] = 0;
    j = (int)((BitLen - MessIndex) / 8.0);
    for (i = 0; i < j; i++)
        padded[i] = mess[(MessIndex / 8) + i];
    padded[i] = 0x80;
    CompressFunction(state, padded, MessIndex & 0x7);
    Squeeze(state, digest);
}

/*
 * =====================================================================================
 *
 *       Filename:  photondriver.c
 *
 *    Description:  driver for the photon hash family
 *
 *        Version:  1.0
 *        Created:  Friday 14,January,2011 09:50:30  SGT
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Jian Guo
 *          Email:  ntu.guo@gmail.com
 *
 * =====================================================================================
 */




template <bool bswap>
static FORCE_INLINE void PHOTON80(const void *in, const size_t len, const size_t seed, void *out)
{

    byte digest[DIGESTSIZE / 8];
    hash(digest,(byte*)in,len*8);
    memcpy(out, digest, 10);
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
REGISTER_FAMILY(photon,
                $.src_url = "https://doi.org/10.1016/j.procs.2014.05.453",
                $.src_status = HashFamilyInfo::SRC_FROZEN);

REGISTER_HASH(PHOTON_80,
              $.desc = "Light-weight One-way Cryptographic Hash Algorithm for Wireless Sensor Network",
              $.hash_flags =
              $.impl_flags =
                  FLAG_HASH_CRYPTOGRAPHIC_WEAK |
                  FLAG_HASH_ENDIAN_INDEPENDENT |
                  FLAG_HASH_NO_SEED,
              $.bits = 80,
              $.hashfn_native = PHOTON80<true>,
              $.hashfn_bswap = PHOTON80<true>);
