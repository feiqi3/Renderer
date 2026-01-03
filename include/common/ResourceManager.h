#ifndef _RESOURCE_MANAGER_H_
#define _RESOURCE_MANAGER_H_

#include "common/CoreDefs.h"
#include "common/Name.h"
#include <string>
#include <memory>
#include <mutex>
#include <map>
namespace Render{

	struct ResourceEntry {
		Name resourceName;
		void* resource;
		std::atomic<u32> refCount = 0;
	};

	class IResourceManager {
	public:
		virtual ~IResourceManager() = default;
		virtual const Name& typeName() const = 0;

		virtual ResourceEntry* acquireOrCreate(const Name& id) = 0;
		virtual ResourceEntry* acquire(const Name& id) = 0;
		virtual void release(const Name& id) = 0;
		friend class ResourceSystem;
	};


	template<typename T>
	class ResourceManager : public IResourceManager {
	public:


		explicit ResourceManager() {}
		virtual ~ResourceManager() = default;

		virtual const Name& typeName() const;

		inline ResourceEntry* acquire(const Name& name) {
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				auto it = m_entries.find(name);
				if (it == m_entries.end()) {
					return nullptr;
				}
				else {
					it->second->refCount++;
					return it->second.get();
				}
			}
		}

		inline ResourceEntry* acquireOrCreate(const Name& name) override {
			{
				ResourceEntry* resource = acquire(name);
				if (resource != nullptr) {
					return resource;
				}
			}
			T* res = loadImpl(name);
			if (!res) return nullptr;

			auto retRaw = res;
			bool needRelease = false;

			ResourceEntry* entry = nullptr;
			{
				//Case when loaded in other thread
				std::lock_guard<std::mutex> lock(m_mutex);
				auto it = m_entries.find(name);
				if (it != m_entries.end()) {
					needRelease = true;
					it->second->refCount++;
					entry = it->second.get();
				}
				else {
					auto e = std::make_unique<ResourceEntry>();
					e->resource = (res);
					e->resourceName = name;
					e->refCount = 1;
					entry = e.get();
					m_entries.insert({ name,std::move(e) });
				}

			}

			if (needRelease) {
				unloadImpl(res);
			}

			return entry;
		}

		inline void release(const Name& id) override {
			T* res = nullptr;
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				auto it = m_entries.find(id);
				if (it == m_entries.end()) return;
				ResourceEntry* entry = it->second.get();
				if (entry->refCount == 0) return;
				entry->refCount--;
				res = (T*)entry->resource;
				if (entry->refCount == 0) {
					m_entries.erase(it);
				}
			}

			unloadImpl(res);
		}

		inline size_t debugRefCount(const Name& id) const {
			auto it = m_entries.find(id);
			return (it == m_entries.end()) ? 0 : it->second->refCount.load();
		}

	protected:
		virtual T* loadImpl(const Name& id) ;

		virtual void unloadImpl(T* /*resource*/) {  }

	private:
		std::mutex m_mutex;
		std::map<Name, std::unique_ptr<ResourceEntry>> m_entries;
	};

}

#endif