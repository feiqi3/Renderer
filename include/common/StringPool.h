#ifndef _STRING_POOL_H_
#define _STRING_POOL_H_
#pragma once
#include "common/CoreDefs.h"
#include "common/Singleton.h"
#include "common/HashFunction.h"
namespace Render {
	
	struct _StringData {
		union {
			const char* mStringVal;
			u64 unique_id;
		};
		u32 mStringLen;
		u32 mHash;
		_StringData(const char* stringPtr,u32 stringLen,u32 hash):mStringVal(stringPtr),mStringLen(stringLen), mHash(hash) {};
	};

	class StringPoolPrivate;
	class StringPool{
	public:
		static StringPool* instance();

		static inline constexpr u32 getStringHash(const char* str, u32 length) {
			return Common::fnv1a_hash(str, length);
		}
		inline _StringData* getOrGenStringData(const char* stringPtr, u32 stringLen) {
			if (!stringPtr || stringLen == 0)return NULL;
			return getOrGenStringData(stringPtr, stringLen,getStringHash(stringPtr,stringLen));
		}

	public:
		StringPool();
		~StringPool();


	private:
		_StringData* getOrGenStringData(const char* stringPtr, u32 stringLen, u32 hash);
		StringPoolPrivate* Dp = 0;
	};
}

#endif