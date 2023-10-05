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
 *     Copyright (c) 2015      Paul G
 *     Copyright (c) 2015-2021 Reini Urban
 *     Copyright (c) 2016      Vlad Egorov
 *     Copyright (c) 2020      Paul Khuong
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
#include "Timing.h"
#include "Hashinfo.h"
#include "TestGlobals.h"
#include "Stats.h" // For FilterOutliers, CalcMean, CalcStdv
#include "Random.h"

#include "SpeedTest.h"

#include <string>
#include <functional>
#include <map>

constexpr int BULK_RUNS   = 16;
constexpr int BULK_TRIALS = 9600;
// constexpr int BULK_SAMPLES = 2;

constexpr int TINY_TRIALS  = 600;   // Timings per hash for small (<128b) keys
constexpr int TINY_SAMPLES = 15000; // Samples per timing run for small sizes

// std::max() isn't constexpr in C++11
constexpr int MAX_TRIALS = (BULK_TRIALS > TINY_TRIALS) ? BULK_TRIALS : TINY_TRIALS;

//-----------------------------------------------------------------------------
// This is functionally a speed test, and so will not inform VCodes,
// since that would affect results too much.

//-----------------------------------------------------------------------------
// We really want the rdtsc() calls to bracket the function call as tightly
// as possible, but that's hard to do portably. We'll try and get as close as
// possible by marking the function as NEVER_INLINE (to keep the optimizer from
// moving it) and marking the timing variables as "volatile register".
//
// Calling the hash function twice seems to improve timing measurement stability
// without affecting branch prediction too much.
NEVER_INLINE static int64_t timehash( HashFn hash, const seed_t seed, void * const key, int len ) {
    volatile int64_t begin, end;
    uint32_t         temp[16];

    begin = timer_start();

    hash(key, len, seed, key);
    hash(key, len, seed, key);

    end = timer_end();

    return end - begin;
}

//-----------------------------------------------------------------------------
// Specialized procedure for small lengths.
//
// This alters the hash key every test, based on the previous hash
// output, in order to:
//   *) make the compiler serialize invocations of the hash function,
//   *) ensure hash invocations would not be computed in parallel //
//      on an out-of-order CPU, and
//   *) try to excercize as many data-dependent paths in the hash code
//      as possible.
//
// By having this return an integer, floating-point math is kept out
// of this routine. This seems to improve timings slightly.
//
// The strange incr value and loop bound are to ensure that the LSB of
// the key is altered every cycle on both big- and little-endian
// machines, without needing an isLE()/isBE() call inside the loop.
// Altering just one byte of the key would do this and would obviate
// the undesirable behavior warned about below, but modifying a single
// byte instead of a whole word is *surprisingly* expensive, even on
// x64 platforms, which leads to unfairly inflated cycle counts.
//
// WARNING: This assumes that at least 4 bytes can be written to key!
NEVER_INLINE static uint64_t timehash_small( HashFn hash, const seed_t seed, uint8_t * const key, int len ) {
    const uint64_t incr = 0x1000001;
    uint64_t       maxi = incr * TINY_SAMPLES;
    volatile unsigned long long int begin, end;
    uint32_t hash_temp[16] = { 0 };

    begin = timer_start();

    for (uint64_t i = 0; i < maxi; i += incr) {
        hash(key, len, seed, hash_temp);
        // It's possible that even with this loop data dependency that
        // hash invocations still would not be fully serialized. Another
        // option is to add lfence instruction to enforce serialization
        // at the CPU level. It's hard to say which one is the most
        // realistic and sensible approach.
        uint32_t j = i ^ hash_temp[0];
        memcpy(key, &j, 4);
    }

    end = timer_end();

    return end - begin;
}

//-----------------------------------------------------------------------------
double stddev;
double rawtimes[MAX_TRIALS];
std::vector<int> sizes( MAX_TRIALS );
std::vector<int> alignments( MAX_TRIALS );
std::map<std::pair<int, int>, std::vector<double>> times;

static double SpeedTest( HashFn hash, seed_t seed, const int trials, const int blocksize,
        const int align, const int maxvarysize, const int maxvaryalign ) {
    static uint64_t callcount = 0;
    Rand r( 444793 + (callcount++));

    uint8_t * buf = new uint8_t[blocksize + 512]; // assumes (align + maxvaryalign) <= 257
    uintptr_t t1  = reinterpret_cast<uintptr_t>(buf);

    r.rand_p(buf, blocksize + 512);
    t1  = (t1 + 255) & UINT64_C(0xFFFFFFFFFFFFFF00);
    t1 += align;

    if (maxvarysize > 0) {
        for (int i = 0; i < trials; i++) {
            sizes.push_back(blocksize - maxvarysize + (i % (maxvarysize + 1)));
        }
        for (int i = trials - 1; i > 0; i--) {
            std::swap(sizes[i], sizes[r.rand_range(i + 1)]);
        }
    } else {
        sizes.insert(sizes.begin(), trials, blocksize);
    }

    if (maxvaryalign > 0) {
        for (int i = 0; i < trials; i++) {
            alignments.push_back((i + 1) % (maxvaryalign + 1));
        }
        for (int i = trials - 1; i > 0; i--) {
            std::swap(alignments[i], alignments[r.rand_range(i + 1)]);
        }
    } else {
        alignments.insert(alignments.begin(), trials, 0);
    }

    //----------
    for (int itrial = 0; itrial < trials; itrial++) {
        int       testsize = sizes[itrial];
        uint8_t * block    = reinterpret_cast<uint8_t *>(t1 + alignments[itrial]);

        double t;
        if (testsize < 128) {
            t = (double)timehash_small(hash, seed, block, testsize) / (double)TINY_SAMPLES;
        } else {
            t = (double)timehash(hash      , seed, block, testsize) / (double)2.0;
        }

        rawtimes[itrial] = t;
    }

    delete [] buf;

    //----------
    for (int itrial = 0; itrial < trials; itrial++) {
        times[std::make_pair(sizes[itrial], alignments[itrial])].push_back(rawtimes[itrial]);
    }

    //----------
    double   avgtotal    = 0.0;
    double   stddevtotal = 0.0;
    unsigned count       = 0;
    unsigned sbmcount    = 0;

    std::map<int, int> summary;
    for (int size = blocksize - maxvarysize; size <= blocksize; size++) {
        for (int align = 0; align <= maxvaryalign; align++) {
            std::vector<double> & timevec = times[std::make_pair(size, align)];
            if (timevec.empty()) { continue; }
            std::sort(timevec.begin(), timevec.end());

            FilterOutliers(timevec);

            avgtotal    += CalcMean(timevec, 0, timevec.size() / 2);
            stddevtotal += CalcStdv(timevec);
            count++;
        }
    }

    sizes.clear();
    alignments.clear();
    times.clear();

    stddev = stddevtotal / count;
    return avgtotal / count;
}

//-----------------------------------------------------------------------------
// 256k blocks seem to give the best results.

static void BulkSpeedTest( const HashInfo * hinfo, seed_t seed, bool vary_align, bool vary_size ) {
    const int    blocksize = 256 * 1024;
    const int    maxvary   = vary_size ? 127 : 0;
    const int    runcount  = hinfo->isVerySlow() ? BULK_RUNS   / 16 : (hinfo->isSlow() ? BULK_RUNS   / 4 : BULK_RUNS  );
    const int    trials    = hinfo->isVerySlow() ? BULK_TRIALS / 16 : (hinfo->isSlow() ? BULK_TRIALS / 4 : BULK_TRIALS);
    const HashFn hash      = hinfo->hashFn(g_hashEndian);

    if (vary_size) {
        printf("Bulk speed test - [%d, %d]-byte keys\n", blocksize - maxvary, blocksize);
    } else {
        printf("Bulk speed test - %d-byte keys\n", blocksize);
    }
    double sumbpc = 0.0;

    volatile double warmup_cycles = SpeedTest(hash, seed, trials, blocksize, 0, 0, 0);

    for (int align = 7; align >= 0; align--) {
        double cycles = 0;
        for (int i = 0; i < runcount; i++) {
            cycles += SpeedTest(hash, seed, trials, blocksize, align, maxvary, 0);
        }
        cycles /= (double)runcount;

        double bestbpc = ((double)blocksize - ((double)maxvary / 2)) / cycles;

        double bestbps = (bestbpc * 3000000000.0 / 1048576.0);
        printf("Alignment  %2d - %6.3f bytes/cycle - %7.2f MiB/sec @ 3 ghz (%10.6f stdv%8.4f%%)\n",
                align, bestbpc, bestbps, stddev, 100.0 * stddev / cycles);
        sumbpc += bestbpc;
    }

    sumbpc = sumbpc / 8.0;
    printf("Average       - %6.3f bytes/cycle - %7.2f MiB/sec @ 3 ghz\n", sumbpc, (sumbpc * 3000000000.0 / 1048576.0));

    // Deliberately not counted in the Average stat, so the two can be directly compared
    if (vary_align) {
        double cycles = 0;
        for (int i = 0; i < runcount; i++) {
            cycles += SpeedTest(hash, seed, trials, blocksize, 0, maxvary, 7);
        }
        cycles /= (double)runcount;

        double bestbpc = ((double)blocksize - ((double)maxvary / 2)) / cycles;

        double bestbps = (bestbpc * 3000000000.0 / 1048576.0);
        printf("Alignment rnd - %6.3f bytes/cycle - %7.2f MiB/sec @ 3 ghz (%10.6f stdv%8.4f%%)\n",
                bestbpc, bestbps, stddev, 100.0 * stddev / cycles);
    }

    fflush(NULL);
}

//-----------------------------------------------------------------------------

static double TinySpeedTest( const HashInfo * hinfo, int maxkeysize, seed_t seed, bool verbose, bool include_vary ) {
    const HashFn hash = hinfo->hashFn(g_hashEndian);
    double       sum  = 0.0;

    printf("Small key speed test - [1, %2d]-byte keys\n", maxkeysize);

    volatile double warmup_cycles = SpeedTest(hash, seed, TINY_TRIALS, maxkeysize, 0, 0, 0);

    for (int i = 1; i <= maxkeysize; i++) {
        volatile int j      = i;
        double       cycles = SpeedTest(hash, seed, TINY_TRIALS, j, 0, 0, 0);
        if (verbose) {
            printf("  %2d-byte keys - %8.2f cycles/hash (%8.6f stdv%8.4f%%)\n",
                    j, cycles, stddev, 100.0 * stddev / cycles);
        }
        sum += cycles;
    }

    sum = sum / (double)maxkeysize;
    printf("Average        - %8.2f cycles/hash\n", sum);

    // Deliberately not counted in the Average stat, so the two can be directly compared
    if (include_vary) {
        double cycles = SpeedTest(hash, seed, TINY_TRIALS, maxkeysize, 0, maxkeysize - 1, 0);
        if (verbose) {
            printf(" rnd-byte keys - %8.2f cycles/hash (%8.6f stdv%8.4f%%)\n", cycles, stddev, 100.0 * stddev / cycles);
        }
    }

    return sum;
}

//-----------------------------------------------------------------------------
bool SpeedTest( const HashInfo * hinfo ) {
    bool result = true;
    Rand r( 633692 );

    printf("[[[ Speed Tests ]]]\n\n");

    const seed_t seed = hinfo->Seed(g_seed ^ r.rand_u64());

    TinySpeedTest(hinfo, 31, seed, true, true);
    printf("\n");

    BulkSpeedTest(hinfo, seed, true, false);
    printf("\n");

    BulkSpeedTest(hinfo, seed, true, true);
    printf("\n");

    return result;
}

//-----------------------------------------------------------------------------
// Does 5 different speed tests to try to summarize hash performance

void ShortSpeedTestHeader( bool verbose ) {
    printf("Bulk results are in bytes/cycle, short results are in cycles/hash\n\n");
    if (verbose) {
        printf("%-25s  %10s  %9s  %17s  %17s  %17s  %17s  \n",
                "Name", "Impl   ", "Bulk  ", "1-8 bytes    ", "9-16 bytes   ", "17-24 bytes   ", "25-32 bytes   ");
        printf("%-25s  %-10s  %9s  %17s  %17s  %17s  %17s  \n",
                "-------------------------", "----------", "---------", "-----------------",
                "-----------------", "-----------------", "-----------------");
    } else {
        printf("%-25s  %9s  %11s  %11s  %11s  %11s  \n",
                "Name", "Bulk  ", "1-8 bytes ", "9-16 bytes", "17-24 bytes", "25-32 bytes");
        printf("%-25s  %9s  %11s  %11s  %11s  %11s  \n",
                "-------------------------", "---------", "-----------",
                "-----------", "-----------", "-----------");
    }
}

void ShortSpeedTest( const HashInfo * hinfo, bool verbose ) {
    const HashFn hash   = hinfo->hashFn(g_hashEndian);
    bool         result = true;
    Rand         r( 321321 );

    const int maxvaryalign    = 7;
    const int basealignoffset = 0;

    printf("%-25s", hinfo->name);
    if (verbose) {
        printf("  %-10s", hinfo->impl);
    }

    const seed_t seed = hinfo->Seed(g_seed ^ r.rand_u64());

    {
        const int baselen    = 256 * 1024;
        const int maxvarylen = 127;

        // Do a warmup to get things into cache
        volatile double warmup_cycles =
                SpeedTest(hash, seed, BULK_TRIALS, baselen, 0, 0, 0);

        // Do a bulk speed test, varying precise block size and alignment
        double cycles = SpeedTest(hash, seed, BULK_TRIALS, baselen, basealignoffset, maxvarylen, maxvaryalign);
        double curbpc = ((double)baselen - ((double)maxvarylen / 2)) / cycles;
        printf("   %7.2f ", curbpc);
    }

    // Do 4 different small block speed tests, averaging over each
    // group of 8 byte lengths (1-8, 9-16, 17-24, 25-31), varying the
    // alignment during each test.
    for (int i = 1; i <= 4; i++) {
        const int baselen     = i * 8;
        double    cycles      = 0.0;
        double    worstdevpct = 0.0;
        for (int j = 0; j < 8; j++) {
            double curcyc = SpeedTest(hash, seed, TINY_TRIALS, baselen + j, basealignoffset, 0, maxvaryalign);
            double devpct = 100.0 * stddev / curcyc;
            cycles += curcyc;
            if (worstdevpct < devpct) {
                worstdevpct = devpct;
            }
        }
        if (verbose) {
            if (worstdevpct < 1.0) {
                printf("   %7.2f [%5.3f] ", cycles / 8.0, worstdevpct);
            } else {
                printf("   %7.2f [%#.4g] ", cycles / 8.0, worstdevpct);
            }
        } else {
            printf("    %7.2f  ", cycles / 8.0);
        }
    }

    printf("\n");
}
