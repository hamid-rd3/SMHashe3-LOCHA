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
#include "Hashinfo.h"
#include "TestGlobals.h"
#include "Stats.h" // for chooseUpToK
#include "Analyze.h"
#include "Instantiate.h"
#include "VCode.h"

#include "ZeroesKeysetTest.h"

//-----------------------------------------------------------------------------
// Keyset 'SeedZeroes' - keys consisting of all zeroes, differing only in length,
// with seeds with up-to-N bits set or cleared.

template <typename hashtype, bool bigseed>
static bool SeedZeroKeyImpl( const HashInfo * hinfo, const size_t maxbits, const size_t keycount, bool verbose ) {
    assert(maxbits < 16);
    const HashFn hash      = hinfo->hashFn(g_hashEndian);
    uint64_t     seeds     = 2 * chooseUpToK(bigseed ? 64 : 32, maxbits);
    uint64_t     totalkeys = seeds * keycount;

    printf("Keyset 'SeedZeroes' - up to %zd-byte keys, seeds with up to %zd set bits - %zd seeds - %zd hashes\n",
            keycount, maxbits, seeds, totalkeys);

    uint8_t * nullblock = new uint8_t[keycount];
    memset(nullblock, 0, keycount);

    addVCodeInput(nullblock, keycount);

    //----------
    std::vector<hashtype> hashes;
    hashes.resize(totalkeys);

    size_t cnt = 0;
    seed_t hseed;

    for (int j = 1; j <= maxbits; j++) {
        uint64_t seed = (UINT64_C(1) << j) - 1;
        bool     done;

        do {
            hseed = hinfo->Seed(seed, false);
            for (int i = 1; i <= keycount; i++) {
                hash(nullblock, i, hseed, &hashes[cnt++]);
            }

            hseed = hinfo->Seed(~seed, false);
            for (int i = 1; i <= keycount; i++) {
                hash(nullblock, i, hseed, &hashes[cnt++]);
            }

            /* Next lexicographic bit pattern, from "Bit Twiddling Hacks" */
            uint64_t t = (seed | (seed - 1)) + 1;
            seed = t | ((((t & -t) / (seed & -seed)) >> 1) - 1);
            done = bigseed ? (seed == ~0) : ((seed >> 32) != 0);
        } while (!done);
    }

    bool result = TestHashList(hashes).drawDiagram(verbose).testDeltas(2 * keycount);
    printf("\n");

    delete [] nullblock;

    recordTestResult(result, "SeedZeroes", keycount);

    addVCodeResult(result);

    return result;
}

//-----------------------------------------------------------------------------

template <typename hashtype>
bool SeedZeroKeyTest( const HashInfo * hinfo, const bool verbose ) {
    bool result = true;

    printf("[[[ Seed 'Zeroes' Tests ]]]\n\n");

    for (auto sz: { 1 * 1024 + 256, 8 * 1024 + 256 }) {
        if (hinfo->is32BitSeed()) {
            result &= SeedZeroKeyImpl<hashtype, false>(hinfo, 2, sz, verbose);
        } else {
            result &= SeedZeroKeyImpl<hashtype,  true>(hinfo, 2, sz, verbose);
        }
    }

    printf("%s\n", result ? "" : g_failstr);

    return result;
}

INSTANTIATE(SeedZeroKeyTest, HASHTYPELIST);
