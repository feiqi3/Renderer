#include <cstdint>
#ifndef HASH_FUNCTION_H
#define HASH_FUNCTION_H
#include "common/CoreDefs.h"
namespace Render::Common {
    inline u32 murmur3_32(const void* key, size_t len, u32 seed)
    {
        const uint8_t* data = static_cast<const uint8_t*>(key);
        const int nblocks = len / 4;

        u32 h1 = seed;
        const u32 c1 = 0xcc9e2d51;
        const u32 c2 = 0x1b873593;

        // body
        const u32* blocks = reinterpret_cast<const u32*>(data);
        for (int i = 0; i < nblocks; ++i) {
            u32 k1 = blocks[i];
            k1 *= c1;
            k1 = (k1 << 15) | (k1 >> 17);
            k1 *= c2;

            h1 ^= k1;
            h1 = (h1 << 13) | (h1 >> 19);
            h1 = h1 * 5 + 0xe6546b64;
        }

        // tail
        const u8* tail = data + nblocks * 4;
        u32 k1 = 0;
        switch (len & 3) {
        case 3: k1 ^= u32(tail[2]) << 16;
        case 2: k1 ^= u32(tail[1]) << 8;
        case 1: k1 ^= u32(tail[0]);
            k1 *= c1;
            k1 = (k1 << 15) | (k1 >> 17);
            k1 *= c2;
            h1 ^= k1;
        }

        // finalization
        h1 ^= len;
        // fmix32
        h1 ^= h1 >> 16;
        h1 *= 0x85ebca6b;
        h1 ^= h1 >> 13;
        h1 *= 0xc2b2ae35;
        h1 ^= h1 >> 16;

        return h1;
    }

    inline constexpr u32 fnv1a_hash(const void* data, size_t len)
    {
        const u8* bytes = static_cast<const u8*>(data);
        u32 hash = 0x811C9DC5u;             // FNV offset basis
        const u32 prime = 0x01000193u;      // FNV prime

        for (size_t i = 0; i < len; ++i) {
            hash ^= bytes[i];
            hash *= prime;
        }
        return hash;
    }

    inline constexpr Render::u64 fnv1a_64(const char* str, size_t len)
    {
        uint64_t hash = 14695981039346656037ULL;  // FNV offset_basis 64

        for (size_t i = 0; i < len; ++i) {
            hash ^= static_cast<uint8_t>(str[i]);
            hash *= 1099511628211ULL;             // FNV prime 64
        }
        return hash;
    }

    inline constexpr Render::u64 fnv1a_64_cstr(const char* str)
    {
        u64 hash = 14695981039346656037ULL;
        while (*str) {
            hash ^= static_cast<uint8_t>(*str++);
            hash *= 1099511628211ULL;
        }
        return hash;
    }
};

#endif