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
 *     obtaining a copy of this software anwd associated documentation
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
#include "Random.h"
#include "Analyze.h"
#include "Histogram.h"
#include "Instantiate.h"
#include "VCode.h"

#include "AvalancheTest.h"

#include <math.h>

#if defined(HAVE_THREADS)
  #include <atomic>
typedef std::atomic<int> a_int;
#else
typedef int a_int;
#endif

//-----------------------------------------------------------------------------
// Flipping a single bit of a key should cause an "avalanche" of changes in
// the hash function's output. Ideally, each output bits should flip 50% of
// the time - if the probability of an output bit flipping is not 50%, that bit
// is "biased". Too much bias means that patterns applied to the input will
// cause "echoes" of the patterns in the output, which in turn can cause the
// hash function to fail to create an even, random distribution of hash values.

template <typename hashtype>
static void calcBiasRange( const HashFn hash, const seed_t seed, std::vector<uint32_t> & bins, const int keybytes,
        const uint8_t * keys, a_int & irepp, const int reps, const bool verbose ) {
    const int keybits = keybytes * 8;

    uint8_t  buf[keybytes];
    hashtype A, B;
    int      irep;

    while ((irep = irepp++) < reps) {
        if (verbose) {
            progressdots(irep, 0, reps - 1, 10);
        }

        ExtBlob K( buf, &keys[keybytes * irep], keybytes );
        hash(K, keybytes, seed, &A);

        uint32_t * cursor = &bins[0];

        for (int iBit = 0; iBit < keybits; iBit++) {
            K.flipbit(iBit);
            hash(K, keybytes, seed, &B);
            K.flipbit(iBit);

            B ^= A;

            cursor = HistogramHashBits(B, cursor);
        }
    }
}

//-----------------------------------------------------------------------------

template <typename hashtype>
static bool AvalancheImpl( HashFn hash, const seed_t seed, const int keybits,
        const int reps, bool drawDiagram, bool drawdots ) {
    Rand r( 48273 + keybits );

    assert((keybits & 7) == 0);

    const int keybytes  = keybits / 8;

    const int hashbytes = sizeof(hashtype);
    const int hashbits  = hashbytes * 8;

    const int arraysize = keybits * hashbits;

    printf("Testing %3d-byte keys, %6d reps........", keybytes, reps);

    std::vector<uint8_t> keys( reps * keybytes );
    for (int i = 0; i < reps; i++) {
        r.rand_p(&keys[i * keybytes], keybytes);
    }
    addVCodeInput(&keys[0], reps * keybytes);

    a_int irep( 0 );

    std::vector<std::vector<uint32_t>> bins( g_NCPU );
    for (unsigned i = 0; i < g_NCPU; i++) {
        bins[i].resize(arraysize);
    }

    if (g_NCPU == 1) {
        calcBiasRange<hashtype>(hash, seed, bins[0], keybytes, &keys[0], irep, reps, drawdots);
    } else {
#if defined(HAVE_THREADS)
        std::thread t[g_NCPU];
        for (int i = 0; i < g_NCPU; i++) {
            t[i] = std::thread {
                calcBiasRange<hashtype>, hash, seed, std::ref(bins[i]),
                keybytes, &keys[0], std::ref(irep), reps, drawdots
            };
        }
        for (int i = 0; i < g_NCPU; i++) {
            t[i].join();
        }
        for (int i = 1; i < g_NCPU; i++) {
            for (int b = 0; b < arraysize; b++) {
                bins[0][b] += bins[i][b];
            }
        }
#endif
    }

    //----------

    bool result = true;

    result &= ReportBias(&bins[0][0], reps, arraysize, hashbits, drawDiagram);

    recordTestResult(result, "Avalanche", keybytes);

    return result;
}

//-----------------------------------------------------------------------------

template <typename hashtype>
bool AvalancheTest( const HashInfo * hinfo, const bool verbose, const bool extra ) {
    const HashFn hash     = hinfo->hashFn(g_hashEndian);
    bool         result   = true;
    bool         drawdots = true; // .......... progress dots

    printf("[[[ Avalanche Tests ]]]\n\n");

    const seed_t seed = hinfo->Seed(g_seed, false, 2);

    std::set<int> testBitsvec = { 24, 32, 40, 48, 56, 64, 72, 80, 96, 128, 160 };
    if (hinfo->bits <= 128) {
        testBitsvec.insert({ 512, 1024 });
    }
    if (extra) {
        testBitsvec.insert({ 192, 224, 256, 320, 384, 448, 512, 1024, 1280, 1536 });
    }

    for (int testBits: testBitsvec) {
        result &= AvalancheImpl<hashtype>(hash, seed, testBits, 300000, verbose, drawdots);
    }

    printf("\n%s\n", result ? "" : g_failstr);

    return result;
}

INSTANTIATE(AvalancheTest, HASHTYPELIST);
