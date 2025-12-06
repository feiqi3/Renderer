#include "common/StringPool.h"
#include "common/HashFunction.h"
#include <cassert>
#include <unordered_map>
#include <memory>
#include <mutex>
namespace Render {
	using StringDataHashTable = std::vector<std::unique_ptr<_StringData>>;
	StringDataHashTable& hashTable(StringDataHashTable* table, size_t buckets, u32 hash)
	{
		assert(buckets > 0);
		size_t hashmod = buckets;
		size_t bucket = hash % hashmod;
		return table[bucket];
	}

	class StringPoolPrivate {
	public:
		StringPoolPrivate(u64 StringPoolSize = 1024 * 256, u32 alignment = 1);
		~StringPoolPrivate();
		_StringData* getOrCreate(const char* str, u32 length);
		_StringData* getOrCreateByHash(const char* str, u32 length,u32 hash);

	public:

		mutable std::mutex mutex_;
		u32 mAlignment;
		u64 mBufferOffset = 0;
		u64 mBufferSize = 0;
		u8* mStringBuffer = 0;
		u32 mBuckets = 1024;
		StringDataHashTable* mStringTable;
	};

	StringPoolPrivate::StringPoolPrivate(u64 StringPoolSize,u32 alignment):mAlignment(alignment),mBufferSize(StringPoolSize), mStringBuffer(new u8[StringPoolSize])
	{
		assert(mAlignment > 0 && ((mAlignment & (mAlignment - 1)) == 0));
		mStringTable = new StringDataHashTable[mBuckets];
	}

	StringPoolPrivate::~StringPoolPrivate()
	{
		delete[] mStringBuffer;
		mStringBuffer = 0;
		delete[] mStringTable;
		mStringTable = 0;
	}

	_StringData* StringPoolPrivate::getOrCreate(const char* str, u32 length)
	{
		if (!str || length == 0)return NULL;
		auto hash = StringPool::getStringHash(str,(size_t)length);
		return getOrCreateByHash(str, length, hash);
	}

	_StringData* StringPoolPrivate::getOrCreateByHash(const char* str, u32 length, u32 hash)
	{
		auto& bucket = hashTable(mStringTable, mBuckets, hash);
		std::lock_guard<std::mutex> guard(mutex_);

		//get existed string data
		{
			for (auto&& sd : bucket) {
				if (sd->mHash != hash || sd->mStringLen != length || strcmp(str, sd->mStringVal) != 0) {
					continue;
				}
				else {
					return sd.get();
				}
			}
		}
		//else create one
		u64 addrEnd = (u64)mStringBuffer + mBufferSize;
		{

			u8* posToStoreStr = mStringBuffer + mBufferOffset;
			u64 addr = (u64)posToStoreStr;
			if (addr % mAlignment) {
				addr = ((addr / mAlignment) + 1) * mAlignment;
			}
			posToStoreStr = (u8*)addr;
			if (addrEnd < addr + length + 1) {
				assert(0 && "String buffer full");
				return nullptr;
			}
			memcpy(posToStoreStr, str, (size_t)(length + 1));
			posToStoreStr[length] = '\0';

			std::unique_ptr<_StringData> sd = std::make_unique<_StringData>(
				(char*)posToStoreStr, length, hash
			);
			auto ret = sd.get();

			bucket.push_back(std::move(sd));
			mBufferOffset = (posToStoreStr - mStringBuffer) + length + 1;
			return ret;
		}
	}


	Render::StringPool::StringPool()
	{
		this->Dp = new StringPoolPrivate();
	}

	Render::StringPool::~StringPool() {
		delete this->Dp;
		Dp = nullptr;
	}

	_StringData* StringPool::getOrGenStringData(const char* stringPtr, u32 stringLen, u32 hash)
	{
		return Dp->getOrCreateByHash(stringPtr, stringLen,hash);
	}

}

