#define XXH_INLINE_ALL
#include "xxhash.h"

#include "fasthash.h"
#include "jody_hash32.h"
#include "jody_hash64.h"

// objsize: 0-0x113 = 276
#include "tifuhash.h"
// objsize: 5f0-85f = 623
#include "floppsyhash.h"

//----------
// These are _not_ hash functions (even though people tend to use crc32 as one...)

void aesrng_init(uint64_t seed);
void aesrng_seed(uint64_t seed, uint64_t hint = 0);
void aesrng32(const void *key, int len, uint32_t seed, void *out);
void aesrng64(const void *key, int len, uint32_t seed, void *out);
void aesrng128(const void *key, int len, uint32_t seed, void *out);
void aesrng160(const void *key, int len, uint32_t seed, void *out);
void aesrng224(const void *key, int len, uint32_t seed, void *out);
void aesrng256(const void *key, int len, uint32_t seed, void *out);
//----------
// General purpose hashes

static inline uint8_t fletcher_bad_seeds(std::vector<uint64_t> &seeds)
{
  seeds = std::vector<uint64_t> { UINT64_C(0) };
  return true;
}
uint64_t fletcher2(const char *key, int len, uint64_t seed);
inline void fletcher2_test(const void *key, int len, uint32_t seed, void *out) {
  *(uint64_t *) out = fletcher2((const char *)key, len, (uint64_t)seed);
}
uint64_t fletcher4(const char *key, int len, uint64_t seed);
inline void fletcher4_test(const void *key, int len, uint32_t seed, void *out) {
  *(uint64_t *) out = fletcher4((const char *)key, len, (uint64_t)seed);
}
uint32_t Bernstein(const char *key, int len, uint32_t seed);
static inline uint8_t Bernstein_bad_seeds(std::vector<uint32_t> &seeds)
{
  seeds = std::vector<uint32_t> { UINT32_C(0) };
  return true;
}
inline void Bernstein_test(const void *key, int len, uint32_t seed, void *out) {
  *(uint32_t *) out = Bernstein((const char *)key, len, seed);
}
uint32_t sdbm(const char *key, int len, uint32_t hash);
static inline uint8_t sdbm_bad_seeds(std::vector<uint32_t> &seeds)
{
  seeds = std::vector<uint32_t> { UINT32_C(0) };
  return true;
}
inline void sdbm_test(const void *key, int len, uint32_t seed, void *out) {
  *(uint32_t *) out = sdbm((const char *)key, len, seed);
}
uint32_t x17(const char *key, int len, uint32_t h);
inline void x17_test(const void *key, int len, uint32_t seed, void *out) {
  *(uint32_t *) out = x17((const char *)key, len, seed);
}
uint32_t JenkinsOOAT(const char *key, int len, uint32_t hash);
inline void JenkinsOOAT_test(const void *key, int len, uint32_t seed, void *out) {
  *(uint32_t *) out = JenkinsOOAT((const char *)key, len, seed);
}
uint32_t JenkinsOOAT_perl(const char *key, int len, uint32_t hash);
inline void JenkinsOOAT_perl_test(const void *key, int len, uint32_t seed, void *out) {
  *(uint32_t *) out = JenkinsOOAT_perl((const char *)key, len, seed);
}
uint32_t GoodOAAT(const char *key, int len, uint32_t hash);
inline void GoodOAAT_test(const void *key, int len, uint32_t seed, void *out) {
  *(uint32_t *) out = GoodOAAT((const char *)key, len, seed);
}
uint32_t MicroOAAT(const char *key, int len, uint32_t hash);
inline void MicroOAAT_test(const void *key, int len, uint32_t seed, void *out) {
  *(uint32_t *) out = MicroOAAT((const char *)key, len, seed);
}
uint32_t SuperFastHash (const char * data, int len, uint32_t hash);
static inline uint8_t SuperFastHash_bad_seeds(std::vector<uint32_t> &seeds)
{
  seeds = std::vector<uint32_t> { UINT32_C(0) };
  return true;
}
inline void SuperFastHash_test(const void *key, int len, uint32_t seed, void *out) {
  *(uint32_t*)out = SuperFastHash((const char*)key, len, seed);
}
uint32_t lookup3(const char *key, int len, uint32_t hash);
inline void lookup3_test(const void *key, int len, uint32_t seed, void *out) {
  *(uint32_t *) out = lookup3((const char *)key, len, seed);
}
uint32_t MurmurOAAT(const char *key, int len, uint32_t hash);
inline void MurmurOAAT_test(const void *key, int len, uint32_t seed, void *out) {
  *(uint32_t *) out = MurmurOAAT((const char *)key, len, seed);
}
uint32_t Crap8(const uint8_t * key, uint32_t len, uint32_t seed);
inline void Crap8_test(const void *key, int len, uint32_t seed, void *out) {
  *(uint32_t *) out = Crap8((const uint8_t *)key, len, seed);
}

//----------
// Used internally as C++
uint32_t MurmurOAAT ( const char * key, int len, uint32_t seed );

inline void jodyhash32_test( const void * key, int len, uint32_t seed, void * out ) {
  *(uint32_t*)out = jody_block_hash32((const jodyhash32_t *)key, (jodyhash32_t) seed, (size_t) len);
}
#ifdef HAVE_INT64
inline void jodyhash64_test( const void * key, int len, uint32_t seed, void * out ) {
  *(uint64_t*)out = jody_block_hash((const jodyhash_t *)key, (jodyhash_t) seed, (size_t) len);
}
#endif

inline void xxHash32_test( const void * key, int len, uint32_t seed, void * out ) {
  // objsize 10-104 + 3e0-5ce: 738
  *(uint32_t*)out = (uint32_t) XXH32(key, (size_t) len, (unsigned) seed);
}
#ifdef HAVE_INT64
inline void xxHash64_test( const void * key, int len, uint32_t seed, void * out ) {
  // objsize 630-7fc + c10-1213: 1999
  *(uint64_t*)out = (uint64_t) XXH64(key, (size_t) len, (unsigned long long) seed);
}
#endif

#define restrict // oddly enough, seems to choke on this keyword
#include "xxh3.h"

#ifdef HAVE_INT64
static inline uint8_t xxh3_bad_seeds(std::vector<uint64_t> &seeds) {
  return false;
}
inline void xxh3_test( const void * key, int len, uint32_t seed, void * out ) {
  // objsize 12d0-15b8: 744
  *(uint64_t*)out = (uint64_t) XXH3_64bits_withSeed(key, (size_t) len, seed);
}
#endif

inline void xxh3low_test( const void * key, int len, uint32_t seed, void * out ) {
  // objsize 12d0-15b8: 744 + 1f50-1f5c: 756
  *(uint32_t*)out = (uint32_t) XXH3_64bits_withSeed(key, (size_t) len, seed);
}

#ifdef HAVE_INT64
inline void xxh128_test( const void * key, int len, uint32_t seed, void * out ) {
  // objsize 1f60-2354: 1012
  *(XXH128_hash_t*)out = XXH128(key, (size_t) len, seed);
}

inline void xxh128low_test( const void * key, int len, uint32_t seed, void * out ) {
  *(uint64_t*)out = (uint64_t) (XXH128(key, (size_t) len, seed).low64);
}
#endif

inline void fasthash32_test ( const void * key, int len, uint32_t seed, void * out ) {
  *(uint32_t*)out = fasthash32(key, (size_t) len, seed);
}
#ifdef HAVE_INT64
inline void fasthash64_test ( const void * key, int len, uint32_t seed, void * out ) {
  *(uint64_t*)out = fasthash64(key, (size_t) len, (uint64_t)seed);
}

#endif

//-----------------------------------------------------------------------------

#if defined(HAVE_AESNI) && defined(__SIZEOF_INT128__) && \
  (defined(__x86_64__) || defined(_M_AMD64) || defined(__i386__)  || defined(_M_IX86))
#define HAVE_MEOW_HASH
#include "meow_hash_x64_aesni.h"
// objsize: 0x84b0-8b94 = 1764
inline void MeowHash128_test(const void *key, int len, unsigned seed, void *out) {
  *(int unsigned *)MeowDefaultSeed = seed;
  meow_u128 h = MeowHash(MeowDefaultSeed, (meow_umm)len, (void*)key);
  ((uint64_t *)out)[0] = MeowU64From(h, 0);
  ((uint64_t *)out)[1] = MeowU64From(h, 1);
}
inline void MeowHash64_test(const void *key, int len, unsigned seed, void *out) {
  *(int unsigned *)MeowDefaultSeed = seed;
  meow_u128 h = MeowHash(MeowDefaultSeed, (meow_umm)len, (void*)key);
  *(uint64_t *)out = MeowU64From(h, 0);
}
inline void MeowHash32_test(const void *key, int len, unsigned seed, void *out) {
  *(int unsigned *)MeowDefaultSeed = seed;
  meow_u128 h = MeowHash(MeowDefaultSeed, (meow_umm)len, (void*)key);
  *(uint32_t *)out = MeowU32From(h, 0);
}
#endif

#include "komihash/komihash.h"
// objsize: 188d0 - 18ba8: 728
inline void komihash_test ( const void * key, int len, unsigned seed, void * out )
{
  *(uint64_t*)out = komihash ((const uint8_t *)key, len, (uint64_t)seed);
}

// objsize: 408dd0 - 4090ae: 734
#include "mx3/mx3.h"
inline void mx3hash64_test ( const void * key, int len, uint32_t seed, void * out ) {
  *(uint64_t*)out = mx3::hash((const uint8_t*)(key), (size_t) len, (uint64_t)seed);
}
inline void mx3rev1hash64_test ( const void * key, int len, uint32_t seed, void * out ) {
  *(uint64_t*)out = mx3::hash_rev1((const uint8_t*)(key), (size_t) len, (uint64_t)seed);
}

// objsize: 63d0 - 6575: 421
extern "C" {
#include "pengyhash.h"
}
inline void pengyhash_test ( const void * key, int len, uint32_t seed, void * out ) {
  *(uint64_t*)out = pengyhash (key, (size_t) len, seed);
}

void nmhash32_test ( const void * key, int len, uint32_t seed, void * out );
void nmhash32x_test ( const void * key, int len, uint32_t seed, void * out );

#ifdef HAVE_INT64
#ifndef HAVE_ALIGNED_ACCESS_REQUIRED

extern "C" {
#undef ROTR32
#undef ROTR64
#include "khash.h"
//objsize: 418eb0-4191d8: 808
inline void khash32_test ( const void *key, int len, uint32_t seed, void *out) {
  uint32_t hash = ~seed;
  uint32_t *dw = (uint32_t*)key;
  const uint32_t *const endw = &((const uint32_t*)key)[len/4];
  while (dw < endw) {
    hash ^= khash32_fn (*dw++, seed, UINT32_C(0xf3bcc908));
  }
  if (len & 3) {
    // the unsafe variant with overflow. see FNV2 for a safe byte-stepper.
    hash ^= khash32_fn (*dw, seed, UINT32_C(0xf3bcc908));
  }
  *(uint32_t*)out = hash;
}
//objsize: 4191e0-419441: 609
inline void khash64_test ( const void *key, int len, uint32_t seed, void *out) {
  uint64_t* dw = (uint64_t*)key;
  const uint64_t *const endw = &((const uint64_t*)key)[len/8];
  const uint64_t seed64 = (uint64_t)seed | UINT64_C(0x6a09e66700000000);
  uint64_t hash = ~seed64;
  while (dw < endw) {
    hash ^= khash64_fn (*dw++, seed64);
  }
  if (len & 7) {
    // unsafe variant with overflow
    hash ^= khash32_fn (*dw, seed, UINT32_C(0xf3bcc908));
  }
  *(uint64_t*)out = hash;
}
}
#endif // HAVE_ALIGNED_ACCESS_REQUIRED


#endif
