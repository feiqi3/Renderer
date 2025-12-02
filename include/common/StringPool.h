#ifndef _STRING_POOL_H_
#define _STRING_POOL_H_
#pragma once
#include "common/CoreDefs.h"
#include "common/Singleton.h"

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
	class StringPool :public Singleton<StringPool>{
	public:

		_StringData* getOrGenStringData(const char* stringPtr, u32 stringLen);

	public:
		StringPool();
		~StringPool();


	private:
		StringPoolPrivate* Dp = 0;
	};
}

#endif