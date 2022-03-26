#define _HASHES_CPP
#include "Platform.h"
#include "Types.h"
#include "Random.h"
#include "VCode.h"

#include "Hashes.h"

#include <cassert>
//#include <emmintrin.h>
//#include <xmmintrin.h>

// ----------------------------------------------------------------------------
// for internal use
#define VCODE_COUNT 3
static XXH3_state_t vcode_states[VCODE_COUNT];

void VCODE_HASH(const void * input, size_t len, unsigned idx) {
    if (idx >= VCODE_COUNT)
        return;
    XXH3_64bits_update(&vcode_states[idx], input, len);
}

static uint32_t VCODE_MASK = 0x0;

void VCODE_INIT(void) {
    for (int i = 0; i < VCODE_COUNT; i++) {
        XXH3_64bits_reset_withSeed(&vcode_states[i], i);
    }
    // This sets VCODE_MASK such that VCODE_FINALIZE() will report a
    // vcode of 0x00000001 if no testing was done.
    VCODE_MASK = VCODE_FINALIZE() ^ 0x1;
}

extern uint32_t g_inputVCode;
extern uint32_t g_outputVCode;
extern uint32_t g_resultVCode;
uint32_t VCODE_FINALIZE(void) {
    if (!g_doVCode) return 1;

    g_inputVCode = (uint32_t)XXH3_64bits_digest(&vcode_states[0]);
    g_outputVCode = (uint32_t)XXH3_64bits_digest(&vcode_states[1]);
    g_resultVCode = (uint32_t)XXH3_64bits_digest(&vcode_states[2]);

    XXH3_state_t finalvcode;
    XXH3_64bits_reset_withSeed(&finalvcode, VCODE_COUNT);

    XXH3_64bits_update(&finalvcode, &g_inputVCode,  sizeof(g_inputVCode));
    XXH3_64bits_update(&finalvcode, &g_outputVCode, sizeof(g_outputVCode));
    XXH3_64bits_update(&finalvcode, &g_resultVCode, sizeof(g_resultVCode));

    return VCODE_MASK ^ XXH3_64bits_digest(&finalvcode);
}

// ----------------------------------------------------------------------------
//fake / bad hashes

//-----------------------------------------------------------------------------
//One - byte - at - a - time hash based on Murmur 's mix

// objsize: 0x540-0x56f: 47
uint32_t MurmurOAAT(const char *key, int len, uint32_t hash)
{
  const uint8_t *data = (const uint8_t *)key;
  const uint8_t *const end = &data[len];

  while (data < end) {
    hash ^= *data++;
    hash *= 0x5bd1e995;
    hash ^= hash >> 15;
  }

  return hash;
}

//-----------------------------------------------------------------------------

// objsize: 0x1090-0x10df: 79
uint32_t
x17(const char *key, int len, uint32_t h)
{
  uint8_t *data = (uint8_t *)key;
  const uint8_t *const end = &data[len];

  while (data < end) {
    h = 17 * h + (*data++ - ' ');
  }
  return h ^ (h >> 16);
}

//64bit, ZFS
//note the original fletcher2 assumes 128bit aligned data, and
//can hereby advance the inner loop by 2 64bit words.
//both fletcher's return 4 words, 256 bit. Both are nevertheless very weak hashes.
// objsize: 0x1120-0x1218: 248
uint64_t
fletcher2(const char *key, int len, uint64_t seed)
{
  uint64_t *dataw = (uint64_t *)key;
  const uint64_t *const endw = &((const uint64_t*)key)[len/8];
  uint64_t A = seed, B = 0;
  for (; dataw < endw; dataw++) {
    A += *dataw;
    B += A;
  }
  if (len & 7) {
    uint8_t *datac = (uint8_t*)dataw; //byte stepper
    const uint8_t *const endc = &((const uint8_t*)key)[len];
    for (; datac < endc; datac++) {
      A += *datac;
      B += A;
    }
  }
  return B;
}

//64bit, ZFS
// objsize: 0x1220-0x1393: 371
uint64_t
fletcher4(const char *key, int len, uint64_t seed)
{
  uint32_t *dataw = (uint32_t *)key;
  const uint32_t *const endw = &((const uint32_t*)key)[len/4];
  uint64_t A = seed, B = 0, C = 0, D = 0;
  while (dataw < endw) {
    A += *dataw++;
    B += A;
    C += B;
    D += C;
  }
  if (len & 3) {
    uint8_t *datac = (uint8_t*)dataw; //byte stepper
    const uint8_t *const endc = &((const uint8_t*)key)[len];
    while (datac < endc) {
      A += *datac++;
      B += A;
      C += B;
      D += C;
    }
  }
  return D;
}

//-----------------------------------------------------------------------------

//also used in perl5 as djb2
// objsize: 0x13a0-0x13c9: 41
uint32_t
Bernstein(const char *key, int len, uint32_t seed)
{
  const uint8_t  *data = (const uint8_t *)key;
  const uint8_t *const end = &data[len];
  while (data < end) {
    //seed = ((seed << 5) + seed) + *data++;
    seed = 33 * seed + *data++;
  }
  return seed;
}

//as used in perl5
// objsize: 0x13a0-0x13c9: 41
uint32_t
sdbm(const char *key, int len, uint32_t hash)
{
  unsigned char  *str = (unsigned char *)key;
  const unsigned char *const end = (const unsigned char *)str + len;
  //note that perl5 adds the seed to the end of key, which looks like cargo cult
  while (str < end) {
    hash = (hash << 6) + (hash << 16) - hash + *str++;
  }
  return hash;
}

//as used in perl5 as one_at_a_time_hard
// objsize: 0x1400-0x1499: 153
uint32_t
JenkinsOOAT(const char *key, int len, uint32_t hash)
{
  unsigned char  *str = (unsigned char *)key;
  const unsigned char *const end = (const unsigned char *)str + len;
  uint64_t	  s = (uint64_t) hash;
  unsigned char  *seed = (unsigned char *)&s;
  //unsigned char seed[8];
  //note that perl5 adds the seed to the end of key, which looks like cargo cult
  while (str < end) {
    hash += (hash << 10);
    hash ^= (hash >> 6);
    hash += *str++;
  }

  hash += (hash << 10);
  hash ^= (hash >> 6);
  hash += seed[4];

  hash += (hash << 10);
  hash ^= (hash >> 6);
  hash += seed[5];

  hash += (hash << 10);
  hash ^= (hash >> 6);
  hash += seed[6];

  hash += (hash << 10);
  hash ^= (hash >> 6);
  hash += seed[7];

  hash += (hash << 10);
  hash ^= (hash >> 6);

  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash = hash + (hash << 15);

  return hash;
}

//as used in perl5 until 5.17(one_at_a_time_old)
// objsize: 0x14a0-0x14e1: 65
uint32_t JenkinsOOAT_perl(const char *key, int len, uint32_t hash)
{
  unsigned char  *str = (unsigned char *)key;
  const unsigned char *const end = (const unsigned char *)str + len;
  while (str < end) {
    hash += *str++;
    hash += (hash << 10);
    hash ^= (hash >> 6);
  }
  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash = hash + (hash << 15);
  return hash;
}

//------------------------------------------------
// One of a smallest non-multiplicative One-At-a-Time function
// that passes whole SMHasher.
// Author: Sokolov Yura aka funny-falcon <funny.falcon@gmail.com>
// objsize: 0x14f0-0x15dd: 237
uint32_t
GoodOAAT(const char *key, int len, uint32_t seed) {
#define grol(x,n) (((x)<<(n))|((x)>>(32-(n))))
#define gror(x,n) (((x)>>(n))|((x)<<(32-(n))))
  unsigned char  *str = (unsigned char *)key;
  const unsigned char *const end = (const unsigned char *)str + len;
  uint32_t h1 = seed ^ 0x3b00;
  uint32_t h2 = grol(seed, 15);
  for (;str != end; str++) {
    h1 += str[0];
    h1 += h1 << 3; // h1 *= 9
    h2 += h1;
    // the rest could be as in MicroOAAT: h1 = grol(h1, 7)
    // but clang doesn't generate ROTL instruction then.
    h2 = grol(h2, 7);
    h2 += h2 << 2; // h2 *= 5
  }
  h1 ^= h2;
  /* now h1 passes all collision checks,
   * so it is suitable for hash-tables with prime numbers. */
  h1 += grol(h2, 14);
  h2 ^= h1; h2 += gror(h1, 6);
  h1 ^= h2; h1 += grol(h2, 5);
  h2 ^= h1; h2 += gror(h1, 8);
  return h2;
#undef grol
#undef gror
}

// MicroOAAT suitable for hash-tables using prime numbers.
// It passes all collision checks.
// Author: Sokolov Yura aka funny-falcon <funny.falcon@gmail.com>
// objsize: 0x15e0-0x1624: 68
uint32_t
MicroOAAT(const char *key, int len, uint32_t seed) {
#define grol(x,n) (((x)<<(n))|((x)>>(32-(n))))
#define gror(x,n) (((x)>>(n))|((x)<<(32-(n))))
  unsigned char  *str = (unsigned char *)key;
  const unsigned char *const end = (const unsigned char *)str + len;
  uint32_t h1 = seed ^ 0x3b00;
  uint32_t h2 = grol(seed, 15);
  while (str < end) {
    h1 += *str++;
    h1 += h1 << 3; // h1 *= 9
    h2 -= h1;
    // unfortunately, clang produces bad code here,
    // cause it doesn't generate rotl instruction.
    h1 = grol(h1, 7);
  }
  return h1 ^ h2;
#undef grol
#undef gror
}

//-----------------------------------------------------------------------------
//Crap8 hash from http://www.team5150.com / ~andrew / noncryptohashzoo / Crap8.html

// objsize: 0x1630-0x1786: 342
uint32_t
Crap8(const uint8_t * key, uint32_t len, uint32_t seed)
{
#define c8fold( a, b, y, z ) { p = (uint32_t)(a) * (uint64_t)(b); y ^= (uint32_t)p; z ^= (uint32_t)(p >> 32); }
#define c8mix( in ) { h *= m; c8fold( in, m, k, h ); }

  const uint32_t  m = 0x83d2e73b, n = 0x97e1cc59, *key4 = (const uint32_t *)key;
  uint32_t	  h = len + seed, k = n + len;
  uint64_t	  p;

  while (len >= 8) {
    c8mix(key4[0]) c8mix(key4[1]) key4 += 2;
    len -= 8;
  }
  if (len >= 4) {
    c8mix(key4[0]) key4 += 1;
    len -= 4;
  }
  if (len) {
    c8mix(key4[0] & ((1 << (len * 8)) - 1))
  }
  c8fold(h ^ k, n, k, k)
  return k;
}
