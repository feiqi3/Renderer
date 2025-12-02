#include <cstdint>
#ifndef HASH_FUNCTION_H
#define HASH_FUNCTION_H
#include "common/CoreDefs.h"
namespace Render::Common {
	u32 murmur3_32(const void* key, size_t len, u32 seed = 0x9747b28c);
	u32 fnv1a_hash(const void* data, size_t len);
	u64 fnv1a_64(const char* str, size_t len);
	u64 fnv1a_64_cstr(const char* str);
};

#endif