/*
 * SHA-2 hash
 * Copyright (C) 2021-2022  Frank J. T. Wojcik
 * Copyright (c) 2019-2021 Reini Urban
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
 *
 * Based on:
 *     SHA-Instrinsics
 *     Written and place in public domain by Jeffrey Walton
 *     Based on code from Intel, and by Sean Gulley for the miTLS project.
 *     Based on code from ARM, and by Johannes Schneiders,
 *     Skip Hovsmith and Barry O'Rourke for the mbedTLS project.
 */

template <bool bswap>
static void SHA256_Transform( uint32_t state[8], const uint8_t data[64] ) {
    __m128i STATE0, STATE1;
    __m128i MSG, TMP;
    __m128i MSG0, MSG1, MSG2, MSG3;
    __m128i ABEF_SAVE, CDGH_SAVE;

    /* Load initial values */
    TMP    = _mm_loadu_si128((const __m128i *)&state[0]);
    STATE1 = _mm_loadu_si128((const __m128i *)&state[4]);

    TMP    = _mm_shuffle_epi32(TMP   , 0xB1);    /* CDAB */
    STATE1 = _mm_shuffle_epi32(STATE1, 0x1B);    /* EFGH */
    STATE0 = _mm_alignr_epi8(TMP, STATE1, 8);    /* ABEF */
    STATE1 = _mm_blend_epi16(STATE1, TMP, 0xF0); /* CDGH */

    /* Save current state */
    ABEF_SAVE = STATE0;
    CDGH_SAVE = STATE1;

    /* Rounds 0-3 */
    MSG0   = _mm_loadu_si128((const __m128i *)(data + 0));
    if (bswap) { MSG0 = mm_bswap32(MSG0); }
    MSG    = _mm_add_epi32(MSG0, _mm_set_epi64x(UINT64_C(0xE9B5DBA5B5C0FBCF), UINT64_C(0x71374491428A2F98)));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    MSG    = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);

    /* Rounds 4-7 */
    MSG1   = _mm_loadu_si128((const __m128i *)(data + 16));
    if (bswap) { MSG1 = mm_bswap32(MSG1); }
    MSG    = _mm_add_epi32(MSG1, _mm_set_epi64x(UINT64_C(0xAB1C5ED5923F82A4), UINT64_C(0x59F111F13956C25B)));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    MSG    = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
    MSG0   = _mm_sha256msg1_epu32(MSG0, MSG1);

    /* Rounds 8-11 */
    MSG2   = _mm_loadu_si128((const __m128i *)(data + 32));
    if (bswap) { MSG2 = mm_bswap32(MSG2); }
    MSG    = _mm_add_epi32(MSG2, _mm_set_epi64x(UINT64_C(0x550C7DC3243185BE), UINT64_C(0x12835B01D807AA98)));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    MSG    = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
    MSG1   = _mm_sha256msg1_epu32(MSG1, MSG2);

    /* Rounds 12-15 */
    MSG3   = _mm_loadu_si128((const __m128i *)(data + 48));
    if (bswap) { MSG3 = mm_bswap32(MSG3); }
    MSG    = _mm_add_epi32(MSG3, _mm_set_epi64x(UINT64_C(0xC19BF1749BDC06A7), UINT64_C(0x80DEB1FE72BE5D74)));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    TMP    = _mm_alignr_epi8(MSG3, MSG2, 4);
    MSG0   = _mm_add_epi32(MSG0, TMP);
    MSG0   = _mm_sha256msg2_epu32(MSG0, MSG3);
    MSG    = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
    MSG2   = _mm_sha256msg1_epu32(MSG2, MSG3);

    /* Rounds 16-19 */
    MSG    = _mm_add_epi32(MSG0, _mm_set_epi64x(UINT64_C(0x240CA1CC0FC19DC6), UINT64_C(0xEFBE4786E49B69C1)));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    TMP    = _mm_alignr_epi8(MSG0, MSG3, 4);
    MSG1   = _mm_add_epi32(MSG1, TMP);
    MSG1   = _mm_sha256msg2_epu32(MSG1, MSG0);
    MSG    = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
    MSG3   = _mm_sha256msg1_epu32(MSG3, MSG0);

    /* Rounds 20-23 */
    MSG    = _mm_add_epi32(MSG1, _mm_set_epi64x(UINT64_C(0x76F988DA5CB0A9DC), UINT64_C(0x4A7484AA2DE92C6F)));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    TMP    = _mm_alignr_epi8(MSG1, MSG0, 4);
    MSG2   = _mm_add_epi32(MSG2, TMP);
    MSG2   = _mm_sha256msg2_epu32(MSG2, MSG1);
    MSG    = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
    MSG0   = _mm_sha256msg1_epu32(MSG0, MSG1);

    /* Rounds 24-27 */
    MSG    = _mm_add_epi32(MSG2, _mm_set_epi64x(UINT64_C(0xBF597FC7B00327C8), UINT64_C(0xA831C66D983E5152)));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    TMP    = _mm_alignr_epi8(MSG2, MSG1, 4);
    MSG3   = _mm_add_epi32(MSG3, TMP);
    MSG3   = _mm_sha256msg2_epu32(MSG3, MSG2);
    MSG    = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
    MSG1   = _mm_sha256msg1_epu32(MSG1, MSG2);

    /* Rounds 28-31 */
    MSG    = _mm_add_epi32(MSG3, _mm_set_epi64x(UINT64_C(0x1429296706CA6351),  UINT64_C(0xD5A79147C6E00BF3)));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    TMP    = _mm_alignr_epi8(MSG3, MSG2, 4);
    MSG0   = _mm_add_epi32(MSG0, TMP);
    MSG0   = _mm_sha256msg2_epu32(MSG0, MSG3);
    MSG    = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
    MSG2   = _mm_sha256msg1_epu32(MSG2, MSG3);

    /* Rounds 32-35 */
    MSG    = _mm_add_epi32(MSG0, _mm_set_epi64x(UINT64_C(0x53380D134D2C6DFC), UINT64_C(0x2E1B213827B70A85)));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    TMP    = _mm_alignr_epi8(MSG0, MSG3, 4);
    MSG1   = _mm_add_epi32(MSG1, TMP);
    MSG1   = _mm_sha256msg2_epu32(MSG1, MSG0);
    MSG    = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
    MSG3   = _mm_sha256msg1_epu32(MSG3, MSG0);

    /* Rounds 36-39 */
    MSG    = _mm_add_epi32(MSG1, _mm_set_epi64x(UINT64_C(0x92722C8581C2C92E), UINT64_C(0x766A0ABB650A7354)));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    TMP    = _mm_alignr_epi8(MSG1, MSG0, 4);
    MSG2   = _mm_add_epi32(MSG2, TMP);
    MSG2   = _mm_sha256msg2_epu32(MSG2, MSG1);
    MSG    = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
    MSG0   = _mm_sha256msg1_epu32(MSG0, MSG1);

    /* Rounds 40-43 */
    MSG    = _mm_add_epi32(MSG2, _mm_set_epi64x(UINT64_C(0xC76C51A3C24B8B70), UINT64_C(0xA81A664BA2BFE8A1)));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    TMP    = _mm_alignr_epi8(MSG2, MSG1, 4);
    MSG3   = _mm_add_epi32(MSG3, TMP);
    MSG3   = _mm_sha256msg2_epu32(MSG3, MSG2);
    MSG    = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
    MSG1   = _mm_sha256msg1_epu32(MSG1, MSG2);

    /* Rounds 44-47 */
    MSG    = _mm_add_epi32(MSG3, _mm_set_epi64x(UINT64_C(0x106AA070F40E3585), UINT64_C(0xD6990624D192E819)));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    TMP    = _mm_alignr_epi8(MSG3, MSG2, 4);
    MSG0   = _mm_add_epi32(MSG0, TMP);
    MSG0   = _mm_sha256msg2_epu32(MSG0, MSG3);
    MSG    = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
    MSG2   = _mm_sha256msg1_epu32(MSG2, MSG3);

    /* Rounds 48-51 */
    MSG    = _mm_add_epi32(MSG0, _mm_set_epi64x(UINT64_C(0x34B0BCB52748774C), UINT64_C(0x1E376C0819A4C116)));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    TMP    = _mm_alignr_epi8(MSG0, MSG3, 4);
    MSG1   = _mm_add_epi32(MSG1, TMP);
    MSG1   = _mm_sha256msg2_epu32(MSG1, MSG0);
    MSG    = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
    MSG3   = _mm_sha256msg1_epu32(MSG3, MSG0);

    /* Rounds 52-55 */
    MSG    = _mm_add_epi32(MSG1, _mm_set_epi64x(UINT64_C(0x682E6FF35B9CCA4F), UINT64_C(0x4ED8AA4A391C0CB3)));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    TMP    = _mm_alignr_epi8(MSG1, MSG0, 4);
    MSG2   = _mm_add_epi32(MSG2, TMP);
    MSG2   = _mm_sha256msg2_epu32(MSG2, MSG1);
    MSG    = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);

    /* Rounds 56-59 */
    MSG    = _mm_add_epi32(MSG2, _mm_set_epi64x(UINT64_C(0x8CC7020884C87814), UINT64_C(0x78A5636F748F82EE)));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    TMP    = _mm_alignr_epi8(MSG2, MSG1, 4);
    MSG3   = _mm_add_epi32(MSG3, TMP);
    MSG3   = _mm_sha256msg2_epu32(MSG3, MSG2);
    MSG    = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);

    /* Rounds 60-63 */
    MSG    = _mm_add_epi32(MSG3, _mm_set_epi64x(UINT64_C(0xC67178F2BEF9A3F7), UINT64_C(0xA4506CEB90BEFFFA)));
    STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
    MSG    = _mm_shuffle_epi32(MSG, 0x0E);
    STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);

    /* Combine state  */
    STATE0 = _mm_add_epi32(STATE0, ABEF_SAVE);
    STATE1 = _mm_add_epi32(STATE1, CDGH_SAVE);

    TMP    = _mm_shuffle_epi32(STATE0, 0x1B);    /* FEBA */
    STATE1 = _mm_shuffle_epi32(STATE1, 0xB1);    /* DCHG */
    STATE0 = _mm_blend_epi16(TMP, STATE1, 0xF0); /* DCBA */
    STATE1 = _mm_alignr_epi8(STATE1, TMP, 8);    /* ABEF */

    /* Save state */
    _mm_storeu_si128((__m128i *)&state[0], STATE0);
    _mm_storeu_si128((__m128i *)&state[4], STATE1);
}
