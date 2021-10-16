/*
 * Copyright (C) 2021-2022  Frank J. T. Wojcik
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
#if defined(HAVE_SSE2) && defined(HAVE_AESNI) && !defined(_MSC_VER)

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <wmmintrin.h>
#include <assert.h>

#include <algorithm>

// ------------------------------------------------------------
// This is bog-standard AES encryption and key expansion
static inline void encrypt(const uint8_t * in, uint8_t * out,  __m128i * round_keys) {
    __m128i tmp;
    tmp = _mm_loadu_si128((const __m128i*)in);
    tmp = _mm_xor_si128(tmp, round_keys[0]);
    for (int j = 1; j <10; j++)
        tmp = _mm_aesenc_si128(tmp, round_keys[j]);
    tmp = _mm_aesenclast_si128(tmp, round_keys[10]);
    _mm_storeu_si128((((__m128i*)out)), tmp);
}

static inline __m128i expand_key_helper( __m128i prev_rkey, __m128i assist ) {
    __m128i rkey = prev_rkey, temp;
    temp = _mm_slli_si128(rkey, 0x4);
    rkey = _mm_xor_si128(rkey, temp);
    temp = _mm_slli_si128(temp, 0x4);
    rkey = _mm_xor_si128(rkey, temp);
    temp = _mm_slli_si128(temp, 0x4);
    rkey = _mm_xor_si128(rkey, temp);

    temp = _mm_shuffle_epi32(assist, 0xff);
    rkey = _mm_xor_si128 (rkey, temp);

    return rkey;
}

#define MKASSIST(x, y) x, _mm_aeskeygenassist_si128(x, y)

static void expand_key(__m128i * round_keys) {
    round_keys[1] = expand_key_helper(MKASSIST(round_keys[0], 0x01));
    round_keys[2] = expand_key_helper(MKASSIST(round_keys[1], 0x02));
    round_keys[3] = expand_key_helper(MKASSIST(round_keys[2], 0x04));
    round_keys[4] = expand_key_helper(MKASSIST(round_keys[3], 0x08));
    round_keys[5] = expand_key_helper(MKASSIST(round_keys[4], 0x10));
    round_keys[6] = expand_key_helper(MKASSIST(round_keys[5], 0x20));
    round_keys[7] = expand_key_helper(MKASSIST(round_keys[6], 0x40));
    round_keys[8] = expand_key_helper(MKASSIST(round_keys[7], 0x80));
    round_keys[9] = expand_key_helper(MKASSIST(round_keys[8], 0x1b));
    round_keys[10] = expand_key_helper(MKASSIST(round_keys[9], 0x36));
}

#undef MKASSIST

// ------------------------------------------------------------
// This is not strictly AES CTR mode, it is based on that plus the ARS
// RNG constructions.

static thread_local __m128i ctr, oldctr;
static const __m128i incr = _mm_set_epi64x(-1ULL,1ULL);
static __m128i round_keys[11]; // only modified on main thread

static uint64_t rnd64(void) {
    __m128i result;
    encrypt((const uint8_t *)&ctr, (uint8_t *)&result, round_keys);
    ctr = _mm_add_epi64(ctr, incr);
    return _mm_cvtsi128_si64x(result);
}

static void rng_ffwd(int ffwd) {
    __m128i ctrfwd = _mm_set_epi64x(-ffwd, ffwd);
    ctr = _mm_add_epi64(ctr, ctrfwd);
}

static void rng_setctr(uint64_t stream, uint64_t seq) {
    ctr = _mm_set_epi64x(stream, seq);
}

/* K1 is golden ratio - 1, K2 is sqrt(3) - 1 */
#define K1 0x9E3779B97F4A7C15ULL
#define K2 0xBB67AE8584CAA73BULL
void aesrng_init(uint64_t seed) {
    round_keys[0] = _mm_set_epi64x(seed + K1, seed + K2);
    expand_key(round_keys);
    ctr = incr;
}

// This variable is _not_ thread-local
static int hash_mode;
// These complications are intended to make this "hash" return the
// same results if threading is enabled or not. It makes the following
// assumptions about the rest of the code:
//
// 1) aesrng_seed() will always be called (at the least) before each
//    group of tests, before any hash() invocation is made in those tests.
// 2) aesrng_seed() may be called in each worker thread or the main thread.
// 3) The hint passed to aesrng_seed() will indicate the start of a
//    possibly-threaded set of tests.
// 4) If threading is being used, the main thread WILL NOT call hash()
//    until another aesrng_seed() call with hint set appropriately.
// 5) The work done by threaded tests is identical to the work done if
//    threading is disabled, but threading may arbitrarily re-order
//    that work.
//
// In this way, the main thread's ctr value just after a set of
// possibly-threaded tests will match the ctr value from just before
// the tests. The value provided during the possibly-threaded tests
// will depend upon the length and first 64 bytes of data being hashed
// and the seed, and not upon the previous ctr value. So the main
// thread's results should be unaffected if threading is enabled or
// disabled, or if the possibly-threaded tests are skipped, and the
// per-thread results should be unaffected by the number of threads.
void aesrng_seed(uint64_t seed, uint64_t hint) {
    if (hash_mode == hint) {
        oldctr = ctr;
    } else {
        hash_mode = hint;
        ctr = oldctr;
    }
}

// This makes the RNG depend on the data to "hash". It is only used
// for possibly-threaded tests.
//
// For hash_mode 1, this just makes random numbers returned be based
// on the seed and first block of data.
//
// Hash_mode 2 is for Avalanche, which is very hard to fool in a
// consistent way, so we have some magic knowledge of how it calls us.
static thread_local uint64_t callcount;
static void rng_keyseq(const void * key, int len, uint64_t seed) {
    if (hash_mode == 2)
        if (callcount-- != 0)
            return;
        else
            callcount = (8 * len);
    uint64_t s = 0;
    memcpy(&s, key, len > 8 ? 8 : len);
    s ^= len * K2;
    seed ^= s * K1;
    s ^= seed * K2;
    rng_setctr(s, seed);
}

template < int nbytes >
static void rng_impl( void * out ) {
    assert((nbytes >= 0) && (nbytes <= 39));
    uint8_t * result = (uint8_t *)out;
    if (nbytes >= 8) {
        uint64_t r = rnd64();
        memcpy(result, &r, 8);
        result += 8;
    }
    if (nbytes >= 16) {
        uint64_t r = rnd64();
        memcpy(result, &r, 8);
        result += 8;
    }
    if (nbytes >= 24) {
        uint64_t r = rnd64();
        memcpy(result, &r, 8);
        result += 8;
    }
    if (nbytes >= 32) {
        uint64_t r = rnd64();
        memcpy(result, &r, 8);
        result += 8;
    }
    if ((nbytes % 8) != 0) {
        uint64_t r = rnd64();
        memcpy(result, &r, nbytes % 8);
    }
}

void aesrng32(const void *key, int len, uint32_t seed, void *out) {
    if (hash_mode != 0)
      rng_keyseq(key, len, seed);
    rng_impl<4>(out);
}

void aesrng64(const void *key, int len, uint32_t seed, void *out) {
    if (hash_mode != 0)
      rng_keyseq(key, len, seed);
    rng_impl<8>(out);
}

void aesrng128(const void *key, int len, uint32_t seed, void *out) {
    if (hash_mode != 0)
      rng_keyseq(key, len, seed);
    rng_impl<16>(out);
}

void aesrng160(const void *key, int len, uint32_t seed, void *out) {
    if (hash_mode != 0)
      rng_keyseq(key, len, seed);
    rng_impl<20>(out);
}

void aesrng224(const void *key, int len, uint32_t seed, void *out) {
    if (hash_mode != 0)
      rng_keyseq(key, len, seed);
    rng_impl<28>(out);
}

void aesrng256(const void *key, int len, uint32_t seed, void *out) {
    if (hash_mode != 0)
      rng_keyseq(key, len, seed);
    rng_impl<32>(out);
}

#endif