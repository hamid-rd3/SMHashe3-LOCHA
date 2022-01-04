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
 *     Copyright (c) 2019-2021 Reini Urban
 *     Copyright (c) 2020      Yann Collet
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
#include "Random.h"
#include "Analyze.h"
#include "Instantiate.h"
#include "VCode.h"

#include "AvalancheTest.h"

#include <cstdio>
#include <cassert>
#include <math.h>

#ifdef HAVE_THREADS
#include <atomic>
typedef std::atomic<int> a_int;
#else
typedef int a_int;
#endif

//-----------------------------------------------------------------------------

static void PrintAvalancheDiagram ( int x, int y, int reps, double scale, int * bins )
{
  const char * symbols = ".123456789X";

  for(int i = 0; i < y; i++)
  {
    printf("[");
    for(int j = 0; j < x; j++)
    {
      int k = (y - i) -1;

      int bin = bins[k + (j*y)];

      double b = double(bin) / double(reps);
      b = fabs(b*2 - 1);

      b *= scale;

      int s = (int)floor(b*10);

      if(s > 10) s = 10;
      if(s < 0) s = 0;

      printf("%c",symbols[s]);
    }

    printf("]\n");
    fflush(NULL);
  }
}

//----------------------------------------------------------------------------

static int maxBias ( int * counts, int buckets, int reps )
{
  int expected = reps / 2;
  int worst = 0;

  for(int i = 0; i < buckets; i++)
  {
    int c = abs(counts[i] - expected);
    if(worst < c)
      worst = c;
  }

  return worst;
}

//-----------------------------------------------------------------------------
// Flipping a single bit of a key should cause an "avalanche" of changes in
// the hash function's output. Ideally, each output bits should flip 50% of
// the time - if the probability of an output bit flipping is not 50%, that bit
// is "biased". Too much bias means that patterns applied to the input will
// cause "echoes" of the patterns in the output, which in turn can cause the
// hash function to fail to create an even, random distribution of hash values.

//-----------------------------------------------------------------------------
// threaded: loop over bins
template < typename hashtype >
static void calcBiasRange ( const pfHash hash, std::vector<int> &bins,
                     const int keybytes, const uint8_t * keys,
                     a_int & irepp, const int reps, const bool verbose )
{
  const int keybits = keybytes * 8;
  const int hashbytes = sizeof(hashtype);

  uint8_t K[keybytes];
  hashtype A,B;
  int irep;

  while ((irep = irepp++) < reps)
  {
    if(verbose) {
      if(irep % (reps/10) == 0) printf(".");
    }

    memcpy(K,&keys[keybytes * irep],keybytes);
    hash(K,keybytes,g_seed,&A);

    int * cursor = &bins[0];

    for(int iBit = 0; iBit < keybits; iBit++)
    {
      flipbit(K,keybytes,iBit);
      hash(K,keybytes,g_seed,&B);
      flipbit(K,keybytes,iBit);

      B ^= A;

      for(int oByte = 0; oByte < hashbytes; oByte++)
      {
        int byte = getbyte(B, oByte);
        for(int oBit = 0; oBit < 8; oBit++)
        {
          (*cursor++) += byte & 1;
          byte >>= 1;
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------

template < typename hashtype >
static bool AvalancheImpl ( pfHash hash, const int keybits, const int reps, bool drawDiagram, bool drawdots )
{
  Rand r(48273);

  assert((keybits & 7)==0);

  const int keybytes = keybits / 8;

  const int hashbytes = sizeof(hashtype);
  const int hashbits = hashbytes * 8;

  const int arraysize = keybits * hashbits;

  printf("Testing %4d-bit keys -> %3d-bit hashes, %6d reps",
         keybits, hashbits, reps);
  //----------
  std::vector<uint8_t> keys(reps * keybytes);
  for (int i = 0; i < reps; i++)
    r.rand_p(&keys[i*keybytes],keybytes);
  addVCodeInput(&keys[0], reps * keybytes);

  a_int irep(0);

  std::vector<std::vector<int> > bins(g_NCPU);
  for (unsigned i = 0; i < g_NCPU; i++) {
      bins[i].resize(arraysize);
  }

  if (g_NCPU == 1) {
      calcBiasRange<hashtype>(hash,bins[0],keybytes,&keys[0],irep,reps,drawdots);
  } else {
#ifdef HAVE_THREADS
      std::thread t[g_NCPU];
      for (int i=0; i < g_NCPU; i++) {
          t[i] = std::thread {calcBiasRange<hashtype>,hash,std::ref(bins[i]),keybytes,&keys[0],std::ref(irep),reps,drawdots};
      }
      for (int i=0; i < g_NCPU; i++) {
          t[i].join();
      }
      for (int i=1; i < g_NCPU; i++)
          for (int b=0; b < arraysize; b++)
              bins[0][b] += bins[i][b];
#endif
  }

  //----------

  int bias = maxBias(&bins[0][0], arraysize, reps);
  bool result = true;

  // Due to threading and memory complications, add the summed
  // avalanche results instead of the hash values. Not ideal, but the
  // "real" way is just too expensive.
  addVCodeOutput(&bins[0][0], arraysize * sizeof(bins[0][0]));
  addVCodeResult(bias);

  result &= ReportBias(bias, reps, arraysize, drawDiagram);

  return result;
}

#if 0
//----------------------------------------------------------------------------
// Tests the Bit Independence Criteron. Stricter than Avalanche, but slow and
// not really all that useful.

template< typename keytype, typename hashtype >
void BicTest1 ( pfHash hash, const int keybit, const int reps, double & maxBias, int & maxA, int & maxB, bool verbose )
{
  Rand r(11938);

  const int keybytes = sizeof(keytype);
  const int hashbytes = sizeof(hashtype);
  const int hashbits = hashbytes * 8;

  std::vector<int> bins(hashbits*hashbits*4,0);

  keytype key;
  hashtype h1,h2;

  for(int irep = 0; irep < reps; irep++)
  {
    if(verbose) {
      if(irep % (reps/10) == 0) printf(".");
    }

    r.rand_p(&key,keybytes);
    hash(&key,keybytes,g_seed,&h1);

    flipbit(key,keybit);
    hash(&key,keybytes,g_seed,&h2);

    hashtype d = h1 ^ h2;

    for(int out1 = 0; out1 < hashbits; out1++)
    for(int out2 = 0; out2 < hashbits; out2++)
    {
      if(out1 == out2) continue;

      uint32_t b = getbit(d,out1) | (getbit(d,out2) << 1);

      bins[(out1 * hashbits + out2) * 4 + b]++;
    }
  }

  if(verbose) printf("\n");

  maxBias = 0;

  for(int out1 = 0; out1 < hashbits; out1++)
  {
    for(int out2 = 0; out2 < hashbits; out2++)
    {
      if(out1 == out2)
      {
        if(verbose) printf("\\");
        continue;
      }

      double bias = 0;

      for(int b = 0; b < 4; b++)
      {
        double b2 = double(bins[(out1 * hashbits + out2) * 4 + b]) / double(reps / 2);
        b2 = fabs(b2 * 2 - 1);

        if(b2 > bias) bias = b2;
      }

      if(bias > maxBias)
      {
        maxBias = bias;
        maxA = out1;
        maxB = out2;
      }

      if(verbose)
      {
        if     (bias < 0.01) printf(".");
        else if(bias < 0.05) printf("o");
        else if(bias < 0.33) printf("O");
        else                 printf("X");
      }
    }

    if(verbose) printf("\n");
  }
}

//----------

template< typename keytype, typename hashtype >
bool BicTest1 ( pfHash hash, const int reps )
{
  const int keybytes = sizeof(keytype);
  const int keybits = keybytes * 8;

  double maxBias = 0;
  int maxK = 0;
  int maxA = 0;
  int maxB = 0;

  for(int i = 0; i < keybits; i++)
  {
    if(i % (keybits/10) == 0) printf(".");

    double bias;
    int a,b;

    BicTest1<keytype,hashtype>(hash,i,reps,bias,a,b,true);

    if(bias > maxBias)
    {
      maxBias = bias;
      maxK = i;
      maxA = a;
      maxB = b;
    }
  }

  printf("Max bias %f - (%3d : %3d,%3d)\n",maxBias,maxK,maxA,maxB);

  // Bit independence is harder to pass than avalanche, so we're a bit more lax here.

  bool result = (maxBias < 0.05);

  return result;
}

//-----------------------------------------------------------------------------
// BIC test variant - iterate over output bits, then key bits. No temp storage,
// but slooooow

template< typename keytype, typename hashtype >
void BicTest2 ( pfHash hash, const int reps, bool verbose = true )
{
  const int keybytes = sizeof(keytype);
  const int keybits = keybytes * 8;
  const int hashbytes = sizeof(hashtype);
  const int hashbits = hashbytes * 8;

  Rand r(11938);

  double maxBias = 0;
  int maxK = 0;
  int maxA = 0;
  int maxB = 0;

  keytype key;
  hashtype h1,h2;

  for(int out1 = 0; out1 < hashbits-1; out1++)
  for(int out2 = out1+1; out2 < hashbits; out2++)
  {
    if(verbose) printf("(%3d,%3d) - ",out1,out2);

    for(int keybit = 0; keybit < keybits; keybit++)
    {
      int bins[4] = { 0, 0, 0, 0 };

      for(int irep = 0; irep < reps; irep++)
      {
        r.rand_p(&key,keybytes);
        hash(&key,keybytes,g_seed,&h1);
        flipbit(key,keybit);
        hash(&key,keybytes,g_seed,&h2);

        hashtype d = h1 ^ h2;

        uint32_t b = getbit(d,out1) | (getbit(d,out2) << 1);

        bins[b]++;
      }

      double bias = 0;

      for(int b = 0; b < 4; b++)
      {
        double b2 = double(bins[b]) / double(reps / 2);
        b2 = fabs(b2 * 2 - 1);

        if(b2 > bias) bias = b2;
      }

      if(bias > maxBias)
      {
        maxBias = bias;
        maxK = keybit;
        maxA = out1;
        maxB = out2;
      }

      if(verbose)
      {
        if     (bias < 0.05) printf(".");
        else if(bias < 0.10) printf("o");
        else if(bias < 0.50) printf("O");
        else                 printf("X");
      }
    }

    // Finished keybit

    if(verbose) printf("\n");
  }

  printf("Max bias %f - (%3d : %3d,%3d)\n",maxBias,maxK,maxA,maxB);
}
#endif /* 0 */

//-----------------------------------------------------------------------------
// BIC test variant - store all intermediate data in a table, draw diagram
// afterwards (much faster)

// The choices for VCode inputs may seem strange here, but they were
// chosen in anticipation of threading this test.

template< typename keytype, typename hashtype >
static bool BicTest3 ( pfHash hash, const int reps, bool verbose = false )
{
  const int keybytes = sizeof(keytype);
  const int keybits = keybytes * 8;
  const int hashbytes = sizeof(hashtype);
  const int hashbits = hashbytes * 8;
  const int pagesize = hashbits*hashbits*4;

  Rand r(11938);

  double maxBias = 0;
  int maxK = 0;
  int maxA = 0;
  int maxB = 0;

  keytype key;
  hashtype h1,h2;

  std::vector<int> bins(keybits*pagesize,0);

  for(int keybit = 0; keybit < keybits; keybit++)
  {
    if(keybit % (keybits/10) == 0) printf(".");

    int * page = &bins[keybit*pagesize];

    for(int irep = 0; irep < reps; irep++)
    {
      r.rand_p(&key,keybytes);
      hash(&key,keybytes,g_seed,&h1);
      addVCodeInput(&key, keybytes);
      addVCodeInput(keybit);
      flipbit(key,keybit);
      hash(&key,keybytes,g_seed,&h2);

      hashtype d = h1 ^ h2;

      for(int out1 = 0; out1 < hashbits-1; out1++)
      for(int out2 = out1+1; out2 < hashbits; out2++)
      {
        int * b = &page[(out1*hashbits+out2)*4];

        uint32_t x = getbit(d,out1) | (getbit(d,out2) << 1);

        b[x]++;
      }
    }
  }

  printf("\n");

  for(int out1 = 0; out1 < hashbits-1; out1++)
  {
    for(int out2 = out1+1; out2 < hashbits; out2++)
    {
      if(verbose) printf("(%3d,%3d) - ",out1,out2);

      for(int keybit = 0; keybit < keybits; keybit++)
      {
        int * page = &bins[keybit*pagesize];
        int * bins = &page[(out1*hashbits+out2)*4];

        double bias = 0;

        for(int b = 0; b < 4; b++)
        {
          double b2 = double(bins[b]) / double(reps / 2);
          b2 = fabs(b2 * 2 - 1);

          if(b2 > bias) bias = b2;
        }

        if(bias > maxBias)
        {
          maxBias = bias;
          maxK = keybit;
          maxA = out1;
          maxB = out2;
        }

        if(verbose)
        {
          if     (bias < 0.01) printf(".");
          else if(bias < 0.05) printf("o");
          else if(bias < 0.33) printf("O");
          else                 printf("X");
        }
      }

      // Finished keybit
      if(verbose) printf("\n");
    }

    if(verbose)
    {
      for(int i = 0; i < keybits+12; i++) printf("-");
      printf("\n");
    }
  }

  addVCodeOutput(&bins[0], keybits*pagesize*sizeof(bins[0]));
  addVCodeResult((uint32_t)(maxBias * 1000.0));
  addVCodeResult(maxK);
  addVCodeResult(maxA);
  addVCodeResult(maxB);

  printf("Max bias %f - (%3d : %3d,%3d)\n",maxBias,maxK,maxA,maxB);

  // Bit independence is harder to pass than avalanche, so we're a bit more lax here.
  bool result = (maxBias < 0.05);
  return result;
}

//-----------------------------------------------------------------------------

template < typename hashtype >
bool BicTest(HashInfo * info, const bool verbose) {
    pfHash hash = info->hash;
    bool result = true;
    bool fewerreps = (info->hashbits > 64 || hash_is_very_slow(hash)) ? true : false;

    printf("[[[ BIC 'Bit Independence Criteria' Tests ]]]\n\n");
    Hash_Seed_init (hash, g_seed);

    if (fewerreps) {
      result &= BicTest3<Blob<128>,hashtype>(hash,100000,verbose);
    } else {
      const long reps = 64000000/info->hashbits;
      //result &= BicTest<uint64_t,hashtype>(hash,2000000);
      result &= BicTest3<Blob<88>,hashtype>(hash,(int)reps,verbose);
    }

    if(!result) printf("*********FAIL*********\n");
    printf("\n");

    return result;
}

INSTANTIATE(BicTest, HASHTYPELIST);

//-----------------------------------------------------------------------------

template < typename hashtype >
bool AvalancheTest(HashInfo* info, const bool verbose, const bool extra) {
    pfHash hash = info->hash;
    bool result = true;
    bool drawdots = true; //.......... progress dots

    printf("[[[ Avalanche Tests ]]]\n\n");
    Hash_Seed_init (hash, g_seed, 2);

    std::vector<int> testBitsvec =
        { 24, 32, 40, 48, 56, 64, 72, 80, 96, 112, 128, 160 };
    if (info->hashbits <= 64) {
        testBitsvec.insert(testBitsvec.end(), { 512, 1024 });
    }
    if (extra) {
        testBitsvec.insert(testBitsvec.end(), { 192, 224, 256, 320, 384, 448, 512, 640,
                                                768, 896, 1024, 1280, 1536 });
    }
    std::sort(testBitsvec.begin(), testBitsvec.end());
    std::unique(testBitsvec.begin(), testBitsvec.end());

    for (int testBits : testBitsvec) {
        result &= AvalancheImpl<hashtype> (hash,testBits,300000,verbose,drawdots);
    }

    if(!result) printf("*********FAIL*********\n");
    printf("\n");

    return result;
}

INSTANTIATE(AvalancheTest, HASHTYPELIST);
