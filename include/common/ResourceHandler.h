#ifndef _RESOURCE_HANDLER_H_
#define _RESOURCE_HANDLER_H_
#include "common/CoreDefs.h"
#include "common/Name.h"
#include "common/ResourceManager.h"
namespace Render {


	template<typename T>
	class ResourceHandle {
	public:
		ResourceHandle(nullptr_t) : entry(nullptr), owner(nullptr), handleRef(0) {}
		ResourceHandle() : entry(nullptr), owner(nullptr), handleRef(0) {}

		ResourceHandle(IResourceManager* mgr, ResourceEntry* e)
			: entry(e), owner(mgr), handleRef(1)
		{
		}

		ResourceHandle(const ResourceHandle& rhs)
			: entry(rhs.entry), owner(rhs.owner), handleRef(rhs.handleRef.load())
		{
			addRef();
		}

		ResourceHandle(ResourceHandle&& rhs) noexcept
			: entry(rhs.entry), owner(rhs.owner), handleRef(rhs.handleRef.load())
		{
			rhs.entry = nullptr;
			rhs.owner = nullptr;
			rhs.handleRef.store(0);
		}

		ResourceHandle& operator=(const ResourceHandle& rhs) {
			if (this != &rhs) {
				release();
				entry = rhs.entry;
				owner = rhs.owner;
				handleRef.store(rhs.handleRef.load());
				addRef();
			}
			return *this;
		}

		ResourceHandle& operator=(ResourceHandle&& rhs) noexcept {
			if (this != &rhs) {
				release();
				entry = rhs.entry;
				owner = rhs.owner;
				handleRef.store(rhs.handleRef.load());
				rhs.entry = nullptr;
				rhs.owner = nullptr;
				rhs.handleRef.store(0);
			}
			return *this;
		}

		~ResourceHandle() {
			release();
		}

		T* operator->() const { return entry ? (T*)entry->resource : nullptr; }
		T& operator*() const { assert(entry && entry->resource); return *((T*)entry->resource); }
		T* get() const { return entry ? (T*)entry->resource : nullptr; }
		bool valid() const { return entry != nullptr && entry->resource != nullptr; }

		void addRef() { handleRef.fetch_add(1, std::memory_order_relaxed); }

		void release() {
			if (!entry) return;

			uint32_t old = handleRef.fetch_sub(1, std::memory_order_acq_rel);
			assert(old > 0);

			if (old == 1 && owner) {
				owner->release(entry->resourceName);
			}

			entry = nullptr;
			owner = nullptr;
		}

		inline explicit operator bool() const noexcept {
			return entry != nullptr;
		}

		inline bool operator==(std::nullptr_t) const noexcept {
			return entry == nullptr;
		}

		inline bool operator!=(std::nullptr_t) const noexcept {
			return entry != nullptr;
		}

	private:
		ResourceEntry* entry;
		IResourceManager* owner;
		std::atomic<uint32_t> handleRef;
	};
}

#endif