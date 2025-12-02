#include "common/StringPool.h"
#include "common/HashFunction.h"
#include <cassert>
#include <unordered_map>
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
		assert(mAlignment > 0 && (mAlignment & (mAlignment - 1) == 0));
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
		auto hash = Common::fnv1a_hash(str,(size_t)length);
		auto& bucket = hashTable(mStringTable, mBuckets, hash);
		//get existed string data
		{
			std::lock_guard<std::mutex> guard(mutex_);
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
			std::lock_guard<std::mutex> guard(mutex_);
			
			u8* posToStoreStr = mStringBuffer + mBufferOffset;
			u64 addr = (u64)posToStoreStr;
			if (addr % mAlignment) {
				addr = ((addr / mAlignment) + 1) * mAlignment;
			}
			posToStoreStr = (u8*)addr;
			if (addrEnd < addr + length + 1) {
				assert("String buffer full");
				return nullptr;
			}
			memcpy(posToStoreStr, str, (size_t)(length + 1));
			posToStoreStr[length] = '\0';

			std::unique_ptr<_StringData> sd = std::make_unique<_StringData>(
				posToStoreStr, length, hash
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

	Render::_StringData* StringPool::getOrGenStringData(const char* stringPtr, u32 stringLen)
	{
		return Dp->getOrCreate(stringPtr, stringLen);
	}

}

