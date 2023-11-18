///////////////////////////////////////////////////////////////////////////////
//
// This file is a combination of own code and code by github.com/AntonJohansson
// from https://github.com/AntonJohansson/StaticMurmur and for simplicity all
// of it is placed under the UNLICENSE (public domain).
//
// The included header (ntpebldr.hpp) retains its own license.
//
///////////////////////////////////////////////////////////////////////////////
//
// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.
//
// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// For more information, please refer to <https://unlicense.org>
//
// SPDX-License-Identifier: Unlicense
//
///////////////////////////////////////////////////////////////////////////////
#ifndef __UNDERHANDED_NTPEBLDR_H_VER__
#define __UNDERHANDED_NTPEBLDR_H_VER__ 2023111821
#if !NTPEBLDR_NO_PRAGMA_ONCE && ((defined(_MSC_VER) && (_MSC_VER >= 1020)) || defined(__MCPP))
#    pragma once
#endif
#if (__cplusplus < 201402L) && (_MSVC_LANG < 201402L)
#    error This header expects a C++14 compatible compiler.
#endif
#include <cstdint>

#if !defined(_M_IX86) && !defined(_M_X64)
#    error This header expects to be built targeting x86-32 or x86-64.
#endif

namespace StaticMurmur3
{
    // The code inside this namespace is derived (only minimal changes from)
    // https://github.com/AntonJohansson/StaticMurmur at commit
    // 7851729fcfb849f0b014e9460f4641ba3747aa96; feel free to pick up the
    // original from there.

    // Returns i:th block (4 bytes) of the data array p as a 32-bit integer.
    // Example:
    //   if const char* p = "abcd" // in hex: 0x61,0x62,0x63,0x64
    //   then get_block(p, 0) returns 0x64636261 as an unsigned integer.
    // Character order is reversed in order to comply with original
    // implementation.
    constexpr uint32_t get_block(char const* p, int i)
    {
        uint32_t block = static_cast<uint8_t>(p[0 + i * 4]) << 0 | static_cast<uint8_t>(p[1 + i * 4]) << 8 | static_cast<uint8_t>(p[2 + i * 4]) << 16 |
                         static_cast<uint8_t>(p[3 + i * 4]) << 24;
        return block;
    }

    constexpr uint32_t rotl32(uint32_t x, int8_t r)
    {
        return (x << r) | (x >> (32 - r));
    }

    constexpr uint32_t fmix32(uint32_t h)
    {
        h ^= h >> 16;
        h *= 0x85ebca6b;
        h ^= h >> 13;
        h *= 0xc2b2ae35;
        h ^= h >> 16;

        return h;
    }

    constexpr uint32_t MurmurHash3_x86_32(char const* key, int const len, uint32_t const seed)
    {
        int const nblocks = len / 4;

        uint32_t h1 = seed;

        constexpr uint32_t const c1 = 0xcc9e2d51;
        constexpr uint32_t const c2 = 0x1b873593;

        //----------
        // body
        for (int i = 0; i < nblocks; i++)
        {
            uint32_t k1 = get_block(key, i);

            k1 *= c1;
            k1 = rotl32(k1, 15);
            k1 *= c2;

            h1 ^= k1;
            h1 = rotl32(h1, 13);
            h1 = h1 * 5 + 0xe6546b64;
        }
        //----------
        // tail

        uint32_t k1 = 0;

        // The "tail" of key are the bytes that do not fit into any block,
        // len%4 is the size of the tail and (tail_start + i) returns the
        // i:th tail byte.
        int const tail_start = len - (len % 4);
        switch (len & 3)
        {
        case 3:
            k1 ^= key[tail_start + 2] << 16;
        case 2:
            k1 ^= key[tail_start + 1] << 8;
        case 1:
            k1 ^= key[tail_start + 0];
            k1 *= c1;
            k1 = rotl32(k1, 15);
            k1 *= c2;
            h1 ^= k1;
        };

        //----------
        // finalization

        h1 ^= len;

        h1 = fmix32(h1);

        return h1;
    }

    template <uint64_t N> constexpr uint32_t static_hash_x86_32(char const (&s)[N], uint32_t seed)
    {
        return MurmurHash3_x86_32(s, N - 1, seed);
    }

} // namespace StaticMurmur3

// BSL-licensed code from here (see file header at the top)
#include "./ntpebldr.hpp"

#endif // __UNDERHANDED_NTPEBLDR_H_VER__
