#include <cstdio>
#if defined(_MSC_VER)
# include <immintrin.h>
# define WIN32_LEAN_AND_MEAN
# include <Windows.h>
typedef UINT32 uint32_t;
typedef UINT8 uint8_t;
#else
# include <stdint.h>
# include <immintrin.h>
#endif
uint32_t state[80];
int main(void) {
    __m512i FOO  = _mm512_set1_epi32(0x04050607);
    __m512i vals = _mm512_loadu_si512((const __m512i *)state);
    FOO  = _mm512_shuffle_epi8(FOO, vals);
    vals = _mm512_min_epi32(vals, FOO);
    vals = _mm512_add_epi32(vals, FOO);
    vals = _mm512_mullo_epi16(vals, FOO);
    _mm512_storeu_si512((__m512i *)(state+8), vals);
}