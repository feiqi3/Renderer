#include <cstdint>
#ifndef HASH_FUNCTION_H
#define HASH_FUNCTION_H

namespace Render::Common {
	uint32_t murmur3_32(const void* key, size_t len, uint32_t seed = 0x9747b28c);
	uint32_t fnv1a_hash(const void* data, size_t len);
};

#endif