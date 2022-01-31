/*
 * wyhash
 * This is free and unencumbered software released into the public
 * domain under The Unlicense (http://unlicense.org/).
 *
 * main repo: https://github.com/wangyi-fudan/wyhash
 * author: 王一 Wang Yi <godspeed_china@yeah.net>
 * contributors: Frank J. T. Wojcik, Reini Urban, Dietrich Epp, Joshua
 * Haberman, Tommy Ettinger, Daniel Lemire, Otmar Ertl, cocowalla,
 * leo-yuriev, Diego Barrios Romero, paulie-g, dumblob, Yann Collet,
 * ivte-ms, hyb, James Z.M. Gao, easyaspi314 (Devin), TheOneric
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a
 * compiled binary, for any purpose, commercial or non-commercial, and
 * by any means.
 *
 * In jurisdictions that recognize copyright laws, the author or
 * authors of this software dedicate any and all copyright interest in
 * the software to the public domain. We make this dedication for the
 * benefit of the public at large and to the detriment of our heirs
 * and successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to
 * this software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 */
#include "Platform.h"
#include "Types.h"
#include "Hashlib.h"

//-----------------------------------------------------------------------------
// Data reading functions, common to 32- and 64-bit hashes
template < bool bswap >
static inline uint64_t _wyr8(const uint8_t * p) {
  return GET_U64<bswap>(p, 0);
}

template < bool bswap >
static inline uint64_t _wyr4(const uint8_t * p) {
  return GET_U32<bswap>(p, 0);
}

static inline uint64_t _wyr3(const uint8_t * p, size_t k) {
  return (((uint64_t)p[0])<<16)|(((uint64_t)p[k>>1])<<8)|p[k-1];
}

//-----------------------------------------------------------------------------
// 128-bit multiply function
//
// All platform-specific code returns the same results for a given
// choice of strict. I.e. for a given set of template parameter
// choices, this function should always give the same answer
// regardless of platform.
static inline uint64_t _wyrot(uint64_t x) { return ROTL64(x, 32); }

template < bool mum32bit, bool strict >
static inline void _wymum(uint64_t *A, uint64_t *B){
  if (mum32bit) {
    uint64_t hh=(*A>>32)*(*B>>32), hl=(*A>>32)*(uint32_t)*B, lh=(uint32_t)*A*(*B>>32), ll=(uint64_t)(uint32_t)*A*(uint32_t)*B;
    if (strict) {
      *A^=_wyrot(hl)^hh; *B^=_wyrot(lh)^ll;
    } else {
      *A=_wyrot(hl)^hh; *B=_wyrot(lh)^ll;
    }
  } else {
#if defined(HAVE_INT128)
    uint128_t r=*A; r*=*B;
    if (strict) {
      *A^=(uint64_t)r; *B^=(uint64_t)(r>>64);
    } else {
      *A=(uint64_t)r; *B=(uint64_t)(r>>64);
    }
#elif defined(NEW_HAVE_UMUL128)
    if (strict) {
      uint64_t  a,  b;
      a=_umul128(*A,*B,&b);
      *A^=a;  *B^=b;
    } else {
      *A=_umul128(*A,*B,B);
    }
#else
    uint64_t ha=*A>>32, hb=*B>>32, la=(uint32_t)*A, lb=(uint32_t)*B, hi, lo;
    uint64_t rh=ha*hb, rm0=ha*lb, rm1=hb*la, rl=la*lb, t=rl+(rm0<<32), c=t<rl;
    lo=t+(rm1<<32); c+=lo<t; hi=rh+(rm0>>32)+(rm1>>32)+c;
    if (strict) {
      *A^=lo;  *B^=hi;
    } else {
      *A=lo;  *B=hi;
    }
#endif
  }
}

//-----------------------------------------------------------------------------
// multiply and xor mix function, aka MUM
template < bool strict >
static inline uint64_t _wymix(uint64_t A, uint64_t B) {
  _wymum<false,strict>(&A,&B);
  return A^B;
}

// wyhash64 main function
template < bool bswap, bool strict >
static inline uint64_t _wyhash64(const void * key, size_t len, uint64_t seed, const uint64_t * secrets) {
  const uint8_t * p = (const uint8_t *)key;
  uint64_t a, b;

  seed ^= secrets[0];

  if (likely(len <= 16)) {
    if (likely(len >= 4)) {
      a = (_wyr4<bswap>(p) << 32)    | _wyr4<bswap>(p+((len>>3)<<2));
      b = (_wyr4<bswap>(p+len-4)<<32)| _wyr4<bswap>(p+len-4-((len>>3)<<2));
    } else if (likely(len>0)) {
      a = _wyr3(p,len);
      b=0;
    } else {
      a = b = 0;
    }
  } else {
    size_t i = len;
    if (unlikely(i>48)) {
      uint64_t see1=seed, see2=seed;
      do {
        seed=_wymix<strict>(_wyr8<bswap>(p)   ^secrets[1],  _wyr8<bswap>(p+8) ^seed);
        see1=_wymix<strict>(_wyr8<bswap>(p+16)^secrets[2],  _wyr8<bswap>(p+24)^see1);
        see2=_wymix<strict>(_wyr8<bswap>(p+32)^secrets[3],  _wyr8<bswap>(p+40)^see2);
        p+=48; i-=48;
      } while(likely(i>48));
      seed ^= see1 ^ see2;
    }
    while (unlikely(i>16)) {
      seed = _wymix<strict>(_wyr8<bswap>(p)^secrets[1], _wyr8<bswap>(p+8)^seed);
      i-=16; p+=16;
    }
    a=_wyr8<bswap>(p+i-16);
    b=_wyr8<bswap>(p+i-8);
  }
  return _wymix<strict>(secrets[1]^len, _wymix<strict>(a^secrets[1], b^seed));
}

//-----------------------------------------------------------------------------
// 32-bit hash function
static inline void _wymix32(uint32_t * A,  uint32_t * B) {
  uint64_t c;
  c  = *A ^ 0x53c5ca59u;
  c *= *B ^ 0x74743c1bu;
  *A = (uint32_t)c;
  *B = (uint32_t)(c >> 32);
}

template < bool bswap >
static inline uint32_t _wyhash32(const void * key, uint64_t len, uint32_t seed) {
  const uint8_t * p = (const uint8_t *)key;
  uint64_t i = len;
  uint32_t see1 = (uint32_t)len;

  seed ^= (uint32_t)(len>>32);
  _wymix32(&seed, &see1);

  for (;i>8;i-=8,p+=8) {
    seed ^= _wyr4<bswap>(p);
    see1 ^= _wyr4<bswap>(p+4);
    _wymix32(&seed, &see1);
  }
  if (i>=4) {
    seed ^= _wyr4<bswap>(p);
    see1 ^= _wyr4<bswap>(p + i - 4);
  } else if (i) {
    seed ^= _wyr3(p, (size_t)i);
  }
  _wymix32(&seed, &see1);
  _wymix32(&seed, &see1);
  return seed ^ see1;
}

//-----------------------------------------------------------------------------
// the default secret parameters
static const uint64_t _wyp[4] = {
  UINT64_C(0xa0761d6478bd642f), UINT64_C(0xe7037ed1a0b428db),
  UINT64_C(0x8ebc6af09c88c6e3), UINT64_C(0x589965cc75374cc3)
};

//-----------------------------------------------------------------------------
template < bool bswap >
void Wyhash32(const void * in, const size_t len, const seed_t seed, void * out) {
  PUT_U32<bswap>(_wyhash32<bswap>(in, (uint64_t)len, (uint32_t)seed), (uint8_t *)out, 0);
}

template < bool bswap, bool strict >
void Wyhash64(const void * in, const size_t len, const seed_t seed, void * out) {
  PUT_U64<bswap>(_wyhash64<bswap,strict>(in, len, (uint64_t)seed, _wyp), (uint8_t *)out, 0);
}

//-----------------------------------------------------------------------------
bool wyhash64_selftest(void) {
  struct {
    const uint64_t hash;
    const char * key;
  } selftests[] = {
    { UINT64_C(0x42bc986dc5eec4d3), "" },
    { UINT64_C(0x84508dc903c31551), "a" },
    { UINT64_C(0x0bc54887cfc9ecb1), "abc" },
    { UINT64_C(0x6e2ff3298208a67c), "message digest" },
    { UINT64_C(0x9a64e42e897195b9), "abcdefghijklmnopqrstuvwxyz" },
    { UINT64_C(0x9199383239c32554), "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" },
    { UINT64_C(0x7c1ccf6bba30f5a5), "12345678901234567890123456789012345678901234567890123456789012345678901234567890" },
  };

  for (int i = 0; i < sizeof(selftests)/sizeof(selftests[0]); i++) {
    uint64_t h;
    if (isLE()) {
      Wyhash64<false,false>(selftests[i].key, strlen(selftests[i].key), i, &h);
    } else {
      Wyhash64<true,false>(selftests[i].key, strlen(selftests[i].key), i, &h);
    }
    if (h != selftests[i].hash) {
      printf("Hash %016llx != expected %016llx for string \"%s\"\n",
	     h, selftests[i].hash, selftests[i].key);
      return false;
    }
  }

  return true;
}


//-----------------------------------------------------------------------------
REGISTER_FAMILY(wyhash);

REGISTER_HASH(wyhash_32,
  $.desc = "wyhash v3, 32-bit native version",
  $.hash_flags =
        FLAG_HASH_SMALL_SEED,
  $.impl_flags =
        FLAG_IMPL_MULTIPLY         |
        FLAG_IMPL_LICENSE_PUBLIC_DOMAIN,
  $.bits = 32,
  $.verification_LE = 0x09DE8066,
  $.verification_BE = 0x9D86BAC7,
  $.hashfn_native = Wyhash32<false>,
  $.hashfn_bswap = Wyhash32<true>,
  $.seedfixfn = excludeBadseeds,
  $.badseeds = { 0x429dacdd, 0xd637dbf3 }
);

REGISTER_HASH(wyhash_64,
  $.desc = "wyhash v3, 64-bit non-strict version",
  $.hash_flags =
	0,
  $.impl_flags =
        FLAG_IMPL_MULTIPLY_64_128  |
        FLAG_IMPL_ROTATE           |
        FLAG_IMPL_LICENSE_PUBLIC_DOMAIN,
  $.bits = 64,
  $.verification_LE = 0x67031D43,
  $.verification_BE = 0x912E4607,
  $.hashfn_native = Wyhash64<false,false>,
  $.hashfn_bswap = Wyhash64<true,false>,
  $.initfn = wyhash64_selftest,
  $.seedfixfn = excludeBadseeds,
  $.badseeds = { 0x14cc886e, 0x1bf4ed84, 0x14cc886e14cc886eULL } // all seeds with those lower bits ?
);

REGISTER_HASH(wyhash_strict_64,
  $.desc = "wyhash v3, 64-bit strict version",
  $.hash_flags =
	0,
  $.impl_flags =
        FLAG_IMPL_MULTIPLY_64_128  |
        FLAG_IMPL_ROTATE           |
        FLAG_IMPL_LICENSE_PUBLIC_DOMAIN,
  $.bits = 64,
  $.verification_LE = 0xA82DBAD7,
  $.verification_BE = 0xDB7957D4,
  $.hashfn_native = Wyhash64<false,true>,
  $.hashfn_bswap = Wyhash64<true,true>
);