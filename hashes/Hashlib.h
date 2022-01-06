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
const HashInfo * findHash(const char * name);
void listHashes(bool nameonly);
bool verifyAllHashes(bool verbose);
HashInfo * convertLegacyHash(LegacyHashInfo * linfo);

#define CONCAT_INNER(x, y) x##y
#define CONCAT(x,y) CONCAT_INNER(x, y)

#define REGISTER_FAMILY(N)                                  \
    static const char * THIS_HASH_FAMILY = #N;              \
    unsigned CONCAT(N,_ref)

#define REGISTER_HASH(N, ...)                               \
    static HashInfo CONCAT(Details,N) = []{                 \
        HashInfo $(#N, THIS_HASH_FAMILY);                   \
        __VA_ARGS__;                                        \
        return $;                                           \
    }();

#define USE_FAMILY(N)                                       \
    extern unsigned CONCAT(N,_ref);                         \
    CONCAT(N,_ref) = 1

static FORCE_INLINE bool isLE(void) {
    uint32_t value = 0xb000000e;
    const void *      addr  = static_cast<const void *>(&value);
    const uint8_t *   lsb   = static_cast<const uint8_t *>(addr);
    return ((*lsb) == 0x0e);
}

static FORCE_INLINE bool isBE(void) {
    uint32_t value = 0xb000000e;
    const void *      addr  = static_cast<const void *>(&value);
    const uint8_t *   lsb   = static_cast<const uint8_t *>(addr);
    return ((*lsb) == 0xb0);
}

// FIXME Make this code properly portable
template < typename T >
static FORCE_INLINE T COND_BSWAP(T value, bool doit) {
    if (!doit || (sizeof(T) < 2)) { return value; }

    switch(sizeof(T)) {
    case 2:  value = __builtin_bswap16((uint16_t)value); break;
    case 4:  value = __builtin_bswap32((uint32_t)value); break;
    case 8:  value = __builtin_bswap64((uint64_t)value); break;
#if 0
#ifdef HAVE_INT128
    case 16: value = __builtin_bswap128((uint128_t)value); break;
#endif
#endif
    default: break;
    }
    return value;
}
