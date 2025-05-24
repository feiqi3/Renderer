#include "common/HashFunction.h"

namespace Render::Common
{
uint32_t murmur3_32(const void* key, size_t len, uint32_t seed)
{
    const uint8_t* data = static_cast<const uint8_t*>(key);
    const int nblocks = len / 4;

    uint32_t h1 = seed;
    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    // body
    const uint32_t* blocks = reinterpret_cast<const uint32_t*>(data);
    for (int i = 0; i < nblocks; ++i) {
        uint32_t k1 = blocks[i];
        k1 *= c1;
        k1 = (k1 << 15) | (k1 >> 17);
        k1 *= c2;

        h1 ^= k1;
        h1 = (h1 << 13) | (h1 >> 19);
        h1 = h1 * 5 + 0xe6546b64;
    }

    // tail
    const uint8_t* tail = data + nblocks * 4;
    uint32_t k1 = 0;
    switch (len & 3) {
    case 3: k1 ^= uint32_t(tail[2]) << 16;
    case 2: k1 ^= uint32_t(tail[1]) << 8;
    case 1: k1 ^= uint32_t(tail[0]);
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

uint32_t fnv1a_hash(const void* data, size_t len)
{
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    uint32_t hash = 0x811C9DC5u;             // FNV offset basis
    const uint32_t prime = 0x01000193u;      // FNV prime

    for (size_t i = 0; i < len; ++i) {
        hash ^= bytes[i];
        hash *= prime;
    }
    return hash;
}



}