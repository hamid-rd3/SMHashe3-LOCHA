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
 *     Copyright (c) 2019-2020 Yann Collet
 *     Copyright (c) 2021      Jim Apple
 *     Copyright (c) 2021      Ori Livneh
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
//-----------------------------------------------------------------------------
// Keyset tests generate various sorts of difficult-to-hash keysets and compare
// the distribution and collision frequency of the hash results against an
// ideal random distribution

#pragma once

#include "Types.h"
#include "Stats.h"
#include "Random.h"   // for rand_p

#include <stdint.h>
#include <inttypes.h>
#include <assert.h>

#include <algorithm>  // for std::swap
#include <string>
#if NCPU > 1 // disable with -DNCPU=0 or 1
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#endif

#undef MAX
#define MAX(x,  y)   (((x) > (y)) ? (x) : (y))

static void printKey(const void* key, size_t len)
{
    const unsigned char* const p = (const unsigned char*)key;
    size_t s;
    printf("\n0x");
    for (s=0; s<len; s++) printf("%02X", p[s]);
    printf("\n  ");
    for (s=0; s<len; s+=8) printf("%-16zu", s);
}

//-----------------------------------------------------------------------------
// Keyset 'Prng'


template< typename hashtype >
void Prn_gen (int nbRn, pfHash hash, std::vector<hashtype> & hashes )
{
  assert(nbRn > 0);

  printf("Generating random numbers by hashing previous output - %d keys\n", nbRn);

  hashtype hcopy;
  memset(&hcopy, 0, sizeof(hcopy));

  // a generated random number becomes the input for the next one
  for (int i=0; i< nbRn; i++) {
      hashtype h;
      hash(&hcopy, sizeof(hcopy), g_seed, &h);
      hashes.push_back(h);
      memcpy(&hcopy, &h, sizeof(h));
  }
}


template< typename hashtype >
bool PrngTest ( hashfunc<hashtype> hash,
                bool testColl, bool testDist, bool drawDiagram )
{

  if (sizeof(hashtype) < 8) {
      printf("Skipping PRNG test; it is designed for hashes >= 64-bits\n\n");
      return true;
  }

  //----------

  std::vector<hashtype> hashes;
  Prn_gen(32 << 20, hash, hashes);

  //----------
  bool result = TestHashList(hashes,drawDiagram,testColl,testDist);

  return result;
}
