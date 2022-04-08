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
#include "Platform.h"
#include "Types.h"
#include "Hashlib.h"
#include "VCode.h"

#include <cstdio>
#include <string>
#include <unordered_map>

//-----------------------------------------------------------------------------
// These are here only so that the linker will consider all the
// translation units as "referred to", so it won't ignore them during
// link time, so that all the global static initializers across all
// the hash functions will actually fire. :-{

unsigned refs();
static unsigned dummy = refs();

//-----------------------------------------------------------------------------
typedef std::unordered_map<std::string, const HashInfo *> HashMap;
typedef std::vector<const HashInfo *> HashMapOrder;

HashMap& hashMap() {
  static HashMap * map = new HashMap;
  return *map;
}

// The sort_order field is intended to be used for people adding
// hashes which should appear inside their family in
// other-than-alphabetical order.
//
// This is overloaded for mock hashes to also override the sorting for
// _family name_, which is not something general users should do.
HashMapOrder defaultSort(HashMap & map) {
    HashMapOrder hashes;
    hashes.reserve(map.size());
    for (auto kv : map) {
        hashes.push_back(kv.second);
    }
    std::sort(hashes.begin(), hashes.end(),
            [](const HashInfo * a, const HashInfo * b) {
                int r;
                if (a->isMock() != b->isMock())               return a->isMock();
                if (a->isMock() && (a->sort_order != b->sort_order))
                                                              return (a->sort_order < b->sort_order);
                if (a->isCrypto() != b->isCrypto())           return a->isCrypto();
                if ((r = strcmp(a->family, b->family)) != 0)  return (r < 0);
                if (a->bits != b->bits)                       return (a->bits < b->bits);
                if (a->sort_order != b->sort_order)           return (a->sort_order < b->sort_order);
                if ((r = strcmp(a->name, b->name)) != 0)      return (r < 0);
                return false;
            });
    return hashes;
}

std::vector<const HashInfo *> findAllHashes(void) {
    HashMapOrder hashes;
    hashes = defaultSort(hashMap());
    return hashes;
}

// FIXME Verify hinfo is all filled out.
unsigned register_hash(const HashInfo * hinfo) {
  static std::unordered_map<uint32_t, const HashInfo *> hashcodes;
  std::string name = hinfo->name;
  // Allow users to lookup hashes by any case
  std::transform(name.begin(), name.end(), name.begin(), ::tolower);

  if (hashMap().find(name) != hashMap().end()) {
    printf("Hash names must be unique.\n");
    printf("\"%s\" (\"%s\") was added multiple times.\n", hinfo->name, name.c_str());
    printf("Note that hash names are using a case-insensitive comparison.\n");
    exit(1);
  }

  if (hinfo->verification_LE != 0) {
      const auto it_LE = hashcodes.find(hinfo->verification_LE);
      if (it_LE == hashcodes.end()) {
          hashcodes[hinfo->verification_LE] = hinfo;
      } else {
          printf("WARNING: Hash with verification code %08x was already registered: %s\n",
                  hinfo->verification_LE, it_LE->second->name);
          printf("         Are you certain %s is a unique implementation?\n", hinfo->name);
      }
  }
  if ((hinfo->verification_BE != 0) && (hinfo->verification_BE != hinfo->verification_LE)) {
      const auto it_BE = hashcodes.find(hinfo->verification_BE);
      if (it_BE == hashcodes.end()) {
          hashcodes[hinfo->verification_BE] = hinfo;
      } else {
          printf("WARNING: Hash with verification code %08x was already registered: %s\n",
                  hinfo->verification_BE, it_BE->second->name);
          printf("         Are you certain %s is a unique implementation?\n", hinfo->name);
      }
  }

  hashMap()[name] = hinfo;
  return hashMap().size();
}

const HashInfo * findHash(const char * name) {
  std::string n = name;
  std::transform(n.begin(), n.end(), n.begin(), ::tolower);
  // Since underscores can't be in names, the user must have meant a dash
  std::replace(n.begin(), n.end(), '_', '-');

  const auto it = hashMap().find(n);
  if (it == hashMap().end()) {
    return NULL;
  }
  return it->second;
}

void listHashes(bool nameonly) {
    if (!nameonly) {
        printf("%-25s %4s  %6s  %-60s\n",
            "Name", "Bits", "Type", "Description");
        printf("%-25s %4s  %6s  %-60s\n",
            "----", "----", "----", "-----------");
    }
    for (const HashInfo * h : defaultSort(hashMap())) {
        if (!nameonly) {
            printf("%-25s %4d  %6s  %-60s\n",
                    h->name, h->bits,
                    h->isMock() ? "MOCK" : (h->isCrypto() ? "CRYPTO" : ""),
                    h->desc);
        } else {
            printf("%s\n", h->name);
        }
    }
}

//-----------------------------------------------------------------------------
static void reportInitFailure(const HashInfo * hinfo) {
    printf("%25s - Hash initialization failed!      ...... FAIL!\n",
            hinfo->name);
}

static bool compareVerification(uint32_t expected, uint32_t actual,
        const char * endstr, const char * name,
        bool verbose, bool prefix) {
    const char * result_str;
    bool result = true;

    if (expected == actual) {
        result_str = (actual != 0) ? "PASS\n" : "INSECURE (should not be 0)\n";
    } else if (expected == 0) {
        result_str = "SKIP (unverifiable)\n";
    } else {
        result_str = "FAIL! (Expected 0x%08x)\n";
        result = false;
    }

    if (verbose) {
        if (prefix) {
            printf("%25s - ", name);
        }
        printf("Verification value %2s 0x%08X ...... ", endstr, actual);
        printf(result_str, expected);
    }

    return result;
}

static const char * endianstr(enum HashInfo::endianness e) {
    switch(e) {
    case HashInfo::ENDIAN_LITTLE     : return "LE"; // "Little endian"
    case HashInfo::ENDIAN_BIG        : return "BE"; // "Big endian"
    case HashInfo::ENDIAN_NATIVE     : return isLE() ? "LE" : "BE";
    case HashInfo::ENDIAN_BYTESWAPPED: return isLE() ? "BE" : "LE";
    case HashInfo::ENDIAN_DEFAULT    : return "CE"; // "Canonical endianness"
    case HashInfo::ENDIAN_NONDEFAULT : return "NE"; // "Non-canonical endianness"
    }
    return NULL; /* unreachable */
}

bool verifyHash(const HashInfo * hinfo, enum HashInfo::endianness endian,
        bool verbose, bool prefix = true) {
    bool result = true;
    const uint32_t actual = hinfo->ComputedVerify(endian);
    const uint32_t expect = hinfo->ExpectedVerify(endian);

    result &= compareVerification(expect, actual, endianstr(endian),
          hinfo->name, verbose, prefix);

    return result;
}

bool verifyAllHashes(bool verbose) {
    bool result = true;
    for (const HashInfo * h : defaultSort(hashMap())) {
        if (!h->Init()) {
            if (verbose) {
                reportInitFailure(h);
            }
            result = false;
        } else if (h->isEndianDefined()) {
            // Verify the hash the canonical way first, and then the
            // other way.
            result &= verifyHash(h, HashInfo::ENDIAN_DEFAULT, verbose);
            result &= verifyHash(h, HashInfo::ENDIAN_NONDEFAULT, verbose);
        } else {
            // Always verify little-endian first, just for consistency
            // for humans looking at the results.
            result &= verifyHash(h, HashInfo::ENDIAN_LITTLE, verbose);
            result &= verifyHash(h, HashInfo::ENDIAN_BIG, verbose);
        }
    }
    printf("\n");
    return result;
}
