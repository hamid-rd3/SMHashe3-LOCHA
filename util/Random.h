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
 */
/*
 * Random number and sequence generation via CBRNG.
 *
 * The Rand object uses the Threefry algorithm as the base RNG. This
 * configures it such that a single 64-bit seed value (either explicitly
 * specified, or derived from user-supplied data) gives a stream of 2^64
 * random numbers. It passes TestU01/BigCrush for both forward and
 * bit-reversed outputs.
 *
 * The important feature of Threefry is that it is fully seekable. This is
 * because it is fundamentally counter-based: instead of storing the output
 * of some sort of state-evolution function to prepare for computing the
 * next random output, it simply increments a counter every iteration. By
 * resetting this counter, random outputs can be arbitrarily replayed at
 * later times, without needing to compute intermediate values.
 *
 * Threefry outputs 4 64-bit random numbers each iteration. The implementation
 * in Random.cpp computes an arbitrary number of these iterations in
 * parallel, to take advantage of vectorization instructions in some CPUs.
 *
 * These outputs are buffered in rngbuf[] until they are needed. Further,
 * the buffer is never refilled until necessary. Data is kept in the buffer
 * in little-endian format. This is to keep output data (both bytes and
 * numbers) the same on differently-endian platforms, and to prioritize the
 * speed of rand_n() over, say, rand_u64(). It lets rand_n() fill large
 * amounts of space quickly, and doesn't penalize numeric generation overly
 * much, as most platforms can byte-swap during the copy into an integer.
 *
 * Public Rand seeking/seeding APIs:
 *
 *   Rand takes a 64-bit seed, or a series of 64-bit values which get
 *   condensed into a 64-bit seed. A seed must be supplied at construction,
 *   and can be changed later via the reseed() method.
 *
 *   For a given seed, the seek(N) method can update the state of the Rand
 *   object to be the same as it would be after N random numbers have been
 *   generated from the initial state. This is much faster than starting
 *   with the initial state and generating and discarding N numbers.
 *
 *   The getoffset() method will return a value N such that the N'th random
 *   number is about to be generated. In this way, the value of getoffset()
 *   can be saved before some amount of random number generation, and then
 *   restored via seek() in order to generate the same sequence of random
 *   numbers another time.
 *
 * Public Rand random number generation APIs:
 *
 *   rand_u64() returns the next random 64-bit integer in native endianness.
 *
 *   rand_range(max) returns a random value in the range [0, max). It is
 *   not completely bias-free, because that would require more than a
 *   single random u64, and it is important that it does to retain the
 *   seekability advantage of this RNG setup. The current bias is negligible.
 *
 *   rand_n(buf, len) fills buf[] with len random bytes. It strictly
 *   follows the sequence of random u64s that are generated, so that it is
 *   possible to seek the RNG "across" a call to rand_n(). It always uses
 *   some multiple of 8 bytes of data internally. Two consecutive calls to
 *   rand_n() are equivalent to one larger call if the first call has a
 *   length evenly divisible by 8. From a given starting state, rand_n()
 *   will emit the same byte byte sequence as the integers given by
 *   rand_u64() when those integers are considered in little-endian order.
 *
 * Public Rand random sequence APIs:
 *
 *   get_seq(seqtype, szelem) produces a RandSeq object, which has its own
 *   API for generating random sequences of items, or even individual items
 *   within those sequences. szelem has slightly different meanings,
 *   depending on the value of seqtype.
 *
 *   RandSeq objects can produce 4 different kinds of sequences:
 *     -) SEQ_DIST_1 will produce a sequence of szelem-byte objects, with
 *        unique values; each value differs from all others by at least 1 bit
 *     -) SEQ_DIST_2 will produce a sequence of szelem-byte objects, where
 *        each value differs from all other by at least 2 bits
 *     -) SEQ_DIST_3 will produce a sequence of szelem-byte objects, where
 *        each value differs from all other by at least 3 bits
 *     -) SEQ_NUM will produce a sequence of 64-bit integers, with values
 *        from 0 through szelem, inclusive.
 *
 *   A given RandSeq object represents only one sequence; it does not
 *   produce a different sequence of items if its APIs are called multiple
 *   times. If multiple sequences are needed, then multiple RandSeq objects
 *   can be created by repeatedly calling get_seq(). For seeking across
 *   get_seq() calls, each call to get_seq() will use RandSeq::RNGU64_USED
 *   (currently == 7) random numbers.
 *
 *   seq_maxelem() will return the maximum possible number of elements in
 *   the sequence type specified. This may be useful to allow a caller to
 *   fall back to a simpler/longer sequence type if their first choice does
 *   not contain as many elements as desired.
 *
 *   While SEQ_NUM and SEQ_DIST_1 have sequence lengths as probably
 *   expected, sequences with higher distance elements can be shorter than
 *   is intuitive. Sequences of elements with a minimum distance of 2
 *   (SEQ_DIST_2) are half the length of SEQ_DIST_1 (so, a sequence of
 *   1-byte elements of type SEQ_DIST_2 has a maximum sequence length of
 *   128, not 256). Sequences of elements with a minimum distance of 3
 *   (SEQ_DIST_3) are even shorter, as shown in the following table:
 *
 *       szelem  |  maxelem
 *      ---------------------
 *         1     |   2** 4  ==         16
 *         2     |   2**11  ==      2,048
 *         3     |   2**19  ==    524,288
 *         4     |   2**26  == 67,108,864
 *         5     |   2**34
 *         6     |   2**42
 *         7     |   2**50
 *         8     |   2**57
 *
 *   These higher-distance sequences can be useful for preventing
 *   collisions in tests which examine the effects of single-bit changes in
 *   hash inputs. With 2 bits of difference between elements, toggling any
 *   bit will never produce another element in the same sequence. This
 *   means that it will never be the case that one iteration of a test will
 *   account for the hash difference between (e.g.) key A and key B while
 *   another iteration will use the difference between key B and key
 *   A. There may be some overlap at the edges (e.g. if C and D are two
 *   elements in the sequence obtained from SEQ_DIST_2, then C^(1<<x) may
 *   equal D^(1<<y) for some x and y, but C^(1<<x) will never equal D.
 *
 *   Further, if 3 bits of difference are possible then that means that no
 *   items in the same test can overlap at all.
 *
 * Public RandSeq random sequence APIs:
 *
 *   write(buf, elem_lo, elem_n) fills buf[] with elem_n elements in its
 *   random sequence, starting with elem_lo. If write() is called with
 *   invalid values (e.g. a request for more elements than exist in the set
 *   specified in the RandSeq object), then it will return false, and no
 *   data will be written to buf[].
 *
 *   Due to details of the current implementation, sequences of elements
 *   larger than 8 bytes do not represent a true random sampling of those
 *   elements. For example, if 12-byte elements are needed for some
 *   SEQ_DIST_* type, then no two elements produced by RandSeq will ever
 *   match in their first 8 bytes. For now this should suffice, and it
 *   seems an acceptable tradeoff for not making this code even more
 *   complex than it is.
 *
 *   maxelem() returns the maximum number of elements that are in the
 *   random sequence, just as in Rand::seq_maxelem().
 */

/*
 * The only somewhat user-serviceable part here. This defines how many
 * copies of the RNG are run in parallel during bulk generation. A good
 * value here is the highest number of 64-bit integers that can fit in the
 * largest vector size your machine supports. For example, AVX2 supports
 * vectors with 4 64-bit integers, so 4 is a good value on AVX2 machines.
 *
 * This value ONLY affects performance. It does not alter any random values
 * that are returned from these objects.
 */
#include <initializer_list>

#define PARALLEL 4

//-----------------------------------------------------------------------------

class RandSeq;

enum RandSeqType : uint32_t {
    SEQ_DIST_1 = 1,
    SEQ_DIST_2 = 2,
    SEQ_DIST_3 = 3,
    SEQ_NUM    = 4,
};

class Rand {
  public:
    // These constants are all defined by the fact that this uses Threefry
    constexpr static unsigned  RANDS_PER_ROUND = 4;
    constexpr static unsigned  RNG_KEYS        = 5;
    constexpr static unsigned  BUFLEN = PARALLEL * RANDS_PER_ROUND;

  private:
    uint64_t  rngbuf[BUFLEN];  // Always in LE byte order
    uint64_t  xseed[RNG_KEYS]; // Threefry keys (xseed[0] is the counter)
    uint64_t  bufidx;          // The next rngbuf[] index to be given out
    uint64_t  rseed;           // The actual seed value
    void refill_buf( void * buf );

    //-----------------------------------------------------------------------------

    inline void update_xseed( void ) {
        // Init keys 1-3 from seed value. This derivation of 3
        // random-ish 64-bit keys from 1 64-bit inputs is completely
        // arbitrary, but is also aesthetically pleasing.
        //
        // Key 0 is the counter, which is left untouched here.
        const uint64_t M1 = UINT64_C(0x9E3779B97F4A7C15);
        const uint64_t M2 = UINT64_C(0x6A09E667F3BCC909);

        xseed[1] = ((ROTR64(rseed, 21) | 1) * M1) + ((ROTR64(rseed, 42) | 1) * M2);
        xseed[2] = ((rseed             | 1) * M1) + ((ROTR64(rseed, 21) | 1) * M2);
        xseed[3] = ((ROTR64(rseed, 42) | 1) * M1) + ((rseed             | 1) * M2);
        // Init key 4 from the Threefish specification.
        const uint64_t K1 = UINT64_C(0x1BD11BDAA9FC1A22);
        xseed[4] = K1 ^ xseed[1] ^ xseed[2] ^ xseed[3];
    }

    //-----------------------------------------------------------------------------

    static inline uint64_t mix( uint64_t a, uint64_t b ) {
        const uint64_t M1 = UINT64_C(0x9E3779B97F4A7C15);
        const uint64_t M2 = UINT64_C(0x6A09E667F3BCC909);
        // Ensure c values in a' = ax + c are odd
        const uint64_t k1 = 1 | ((b & 0xFFFFFFFF) << 1);
        const uint64_t k2 = 1 | (b >> 31);
        a = M1 * a + M2 * k1;
        a = M1 * a + M2 * k2;
        return a;
    }

    //-----------------------------------------------------------------------------

  public:
    Rand( uint64_t seed = 0 ) {
        reseed(seed);
    }

    Rand( std::initializer_list<uint64_t> seeds ) {
        reseed(seeds);
    }

    inline void reseed( uint64_t seed ) {
        rseed = seed;
        seek(0);
        update_xseed();
    }

    inline void reseed( std::initializer_list<uint64_t> seeds ) {
        rseed = 1;
        for (uint64_t next: seeds) {
            rseed = mix(rseed, next);
        }
        reseed(rseed);
    }

    inline void seek( uint64_t offset ) {
        xseed[0] = offset / RANDS_PER_ROUND;
        bufidx   = BUFLEN + (offset % RANDS_PER_ROUND);
    }

    inline uint64_t getoffset( void ) const {
        return (xseed[0] * RANDS_PER_ROUND) + bufidx - BUFLEN;
    }

    //-----------------------------------------------------------------------------

    inline uint64_t rand_u64( void ) {
        if (expectp(bufidx >= BUFLEN, 1.0 / BUFLEN)) {
            refill_buf(rngbuf);
            bufidx -= BUFLEN;
        }
        return COND_BSWAP(rngbuf[bufidx++], isBE());
    }

    inline uint32_t rand_range( uint32_t max ) {
        uint32_t lzbits = clz4(max | 1);
        uint64_t r      = rand_u64() >> (64 - lzbits);

        return (r * max) >> lzbits;
    }

    void rand_n( void * buf, size_t bytes );

    //-----------------------------------------------------------------------------

    RandSeq get_seq( enum RandSeqType seqtype, const uint32_t szelem );

    static uint64_t seq_maxelem( enum RandSeqType seqtype, const uint32_t szelem );

    //-----------------------------------------------------------------------------

    bool operator == ( const Rand & k ) const {
        if (memcmp(&xseed[1], &k.xseed[1], sizeof(xseed) - sizeof(xseed[0])) != 0) {
            return false;
        }
        if (rseed != k.rseed) {
            return false;
        }
        if (getoffset() != k.getoffset()) {
            return false;
        }
        return true;
    }
}; // class Rand

//-----------------------------------------------------------------------------

class RandSeq {
  public:
    // Even though, in theory, only 2 full Feistel rounds are needed for
    // encryption, some smaller block sizes used in Random.cpp require more
    // rounds to get sufficient uniformity of permutations.
    //
    // The "- 2" in RNGU64_USED is 1 for the counter and 1 for the
    // Threefish-defined key, neither of which are random.
    constexpr static unsigned  FEISTEL_MAXROUNDS = 4;
    constexpr static unsigned  RNGU64_USED       = FEISTEL_MAXROUNDS + Rand::RNG_KEYS - 2;

  private:
    friend class Rand;

    // fkeys[] is used in a Feistel cipher to generate random
    // variable-width permutations. rkeys[] is used similarly to xseed[] in
    // Rand objects. rkeys[1] is also used as an additional 64-bit random
    // number for some things related to those permutations.
    uint32_t          fkeys[FEISTEL_MAXROUNDS * 2];
    uint64_t          rkeys[Rand::RNG_KEYS];
    uint32_t          szelem;
    enum RandSeqType  type;

    template <unsigned mindist>
    void fill_elem( uint8_t * out, const uint64_t elem_lo, const uint64_t elem_hi, const uint64_t elem_stride );

    // A bare RandSeq() object is unusable; initialize via Rand::get_seq().
    RandSeq() {}

  public:
    bool write( void * buf, const uint64_t elem_lo, const uint64_t elem_n );

    inline uint64_t maxelem( void ) {
        return Rand::seq_maxelem(type, szelem);
    }
}; // class RandSeq

//-----------------------------------------------------------------------------

void RandTest( const unsigned runs );
void RandBenchmark( void );
