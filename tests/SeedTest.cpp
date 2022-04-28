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
#include "Platform.h"
#include "TestGlobals.h"
#include "Stats.h"       // For chooseUpToK
#include "Analyze.h"
#include "Instantiate.h"
#include "VCode.h"

#include "SeedTest.h"

#include <cassert>

//-----------------------------------------------------------------------------
// Keyset 'Seed' - hash "the quick brown fox..." using different seeds

template < typename hashtype, uint32_t seedbits, bool bigseed >
static bool SeedTestImpl(const HashInfo * hinfo, bool drawDiagram) {
  assert(seedbits <= 31);
  const HashFn hash = hinfo->hashFn(g_hashEndian);
  const int totalkeys = 1 << seedbits;
  const int hibits = seedbits >> 1;
  const int lobits = seedbits - hibits;
  const int shiftbits = bigseed ? (64 - hibits) : (32 - hibits);

  printf("Keyset 'Seed' - %d keys\n", totalkeys);

  const char text[64] = "The quick brown fox jumps over the lazy dog";
  const int len = (int)strlen(text);

  addVCodeInput(text, len);
  addVCodeInput(totalkeys);

  //----------

  std::vector<hashtype> hashes;

  hashes.resize(totalkeys);

  for(seed_t i = 0; i < (1 << hibits); i++) {
      for(seed_t j = 0; j < (1 << lobits); j++) {
          const seed_t seed = (i << shiftbits) + j;
          const seed_t hseed = hinfo->Seed(seed, true);
          hash(text, len, hseed, &hashes[(i << lobits) + j]);
      }
  }

  bool result = TestHashList(hashes,drawDiagram);
  printf("\n");

  recordTestResult(result, "Seed", "Seq");

  addVCodeResult(result);

  return result;
}

//-----------------------------------------------------------------------------
// Keyset 'SparseSeed' - hash "sphinx of black quartz..." using seeds with few
// bits set/cleared

template < typename hashtype, bool bigseed >
static bool SparseSeedTestImpl(const HashInfo * hinfo, uint32_t maxbits, bool drawDiagram) {
  assert(maxbits < 16);
  const HashFn hash = hinfo->hashFn(g_hashEndian);
  uint64_t totalkeys = 2 + 2*chooseUpToK(bigseed ? 64 : 32, maxbits);
  uint64_t cnt = 0;

  printf("Keyset 'SparseSeed' - %ld keys\n", totalkeys);

  const char text[64] = "Sphinx of black quartz, judge my vow";
  const int len = (int)strlen(text);

  addVCodeInput(text, len);
  addVCodeInput(totalkeys);

  //----------

  std::vector<hashtype> hashes;
  hashes.resize(totalkeys);

  seed_t seed;

  seed = hinfo->Seed(0, true);
  hash(text, len, seed, &hashes[cnt++]);

  seed = hinfo->Seed(~0, true);
  hash(text, len, seed, &hashes[cnt++]);

  for(seed_t i = 1; i <= maxbits; i++) {
      uint64_t seed = (UINT64_C(1) << i) - 1;
      bool done;

      do {
          seed_t hseed;
          hseed = hinfo->Seed(seed, true);
          hash(text, len, hseed, &hashes[cnt++]);

          hseed = hinfo->Seed(~seed, true);
          hash(text, len, hseed, &hashes[cnt++]);

          /* Next lexicographic bit pattern, from "Bit Twiddling Hacks" */
          uint64_t t = (seed | (seed - 1)) + 1;
          seed = t | ((((t & -t) / (seed & -seed)) >> 1) - 1);
          done = bigseed ? (seed == ~0) : ((seed >> 32) != 0);
      } while (!done);
  }

  bool result = TestHashList(hashes,drawDiagram);
  printf("\n");

  recordTestResult(result, "Seed", "Sparse");

  addVCodeResult(result);

  return result;
}

//-----------------------------------------------------------------------------

template < typename hashtype >
bool SeedTest(const HashInfo * hinfo, const bool verbose) {
    bool result = true;

    printf("[[[ Keyset 'Seed' Tests ]]]\n\n");

    if (hinfo->is32BitSeed()) {
      result &= SeedTestImpl<hashtype,22,false>( hinfo, verbose );
      result &= SparseSeedTestImpl<hashtype,false>( hinfo, 7, verbose );
    } else {
      result &= SeedTestImpl<hashtype,22,true>( hinfo, verbose );
      result &= SparseSeedTestImpl<hashtype,true>( hinfo, 5, verbose );
    }

    printf("%s\n", result ? "" : g_failstr);

    return result;
}

INSTANTIATE(SeedTest, HASHTYPELIST);
