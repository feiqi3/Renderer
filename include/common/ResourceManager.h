#ifndef _RESOURCE_MANAGER_H_
#define _RESOURCE_MANAGER_H_

#include "common/CoreDefs.h"
#include "common/Name.h"
#include <functional>
#include <string>
#include <memory>
#include <mutex>
#include <map>
namespace Render{

	enum class ResourceLifetime : u8 {
		Transient,
		Persistent,     // Do not destroy when ref count == 0
		Manual          // Can only be destroyed  manully
	};

	enum class ResourceState
	{
		Invalid,
		Unloaded,
		Loading,
		Loaded,
		Failed,
		Reloading
	};

	struct ResourceMemory {
		u32 cpuMemory;
		u32 gpuMemory;
	};

	class IResource {
	public:
		inline ResourceState GetState() {
			return mState;
		}
		inline virtual bool IsReady() const {
			return mState == ResourceState::Loaded;
		};
		virtual const Name& getTypeName() const = 0;
		virtual ResourceMemory getMemory() const = 0;
	public:

		virtual ~IResource() {}
		virtual void OnLoaded() {}
		virtual void OnReloadBegin() {}
		virtual void OnReloadEnd() {}
		virtual void OnUnload() {}
	protected:
		ResourceState mState = ResourceState::Invalid;
	};

	using UserDeletor = std::function<bool(const Name&, IResource*)>;

	struct ResourceEntry {
		Name resourceName;
		IResource* resource = nullptr;
		std::atomic<u32> refCount = 0;
		ResourceLifetime lifeTimeCtrl = ResourceLifetime::Transient;
		UserDeletor	deletor = nullptr;
	};

	class IResourceManager {
	public:
		virtual ~IResourceManager() = default;
		virtual const Name& typeName() const = 0;
		virtual ResourceEntry* registerResource(const Name& id, IResource* resource, ResourceLifetime lifeTimeCtr,UserDeletor deletor) = 0;
		virtual void createNecessaryPersistenceResources() = 0;
		virtual ResourceEntry* create(const Name& id, ResourceLifetime lifeTimeCtr) = 0;
		virtual void		   destroy(const Name& id) = 0;//force delete.
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

		virtual const Name& typeName() const  = 0;

		inline virtual ResourceEntry* registerResource(const Name& id, IResource* resource, ResourceLifetime lifeTimeCtr, UserDeletor deletor) {
			const Name& tarName = resource->getTypeName();
			if (tarName != this->typeName()) {
				assert(0);
				throw std::runtime_error("Resource Type Error");
			}
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				auto it = m_entries.find(id);
				if (it != m_entries.end()) {
					return nullptr;
				}
				auto entry = std::make_unique<ResourceEntry>();
				entry->resourceName = id;
				entry->resource = resource;
				entry->refCount = 1;
				entry->lifeTimeCtrl = lifeTimeCtr;
				entry->deletor = deletor;
				auto ret = entry.get();
				this->m_entries.insert({ id, std::move(entry) });
				return ret;
			}
		};

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

		inline ResourceEntry* create(const Name& name, ResourceLifetime lifeTimeCtr) {
			T* res = loadImpl(name);
			if (!res) return nullptr;

			auto retRaw = res;
			bool needRelease = false;

			ResourceEntry* entry = nullptr;
			{
				//Case when loaded in other thread or loaded repeatly
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
					e->lifeTimeCtrl = lifeTimeCtr;
					entry = e.get();
					m_entries.insert({ name,std::move(e) });
				}

			}

			if (needRelease) {
				unloadImpl(res);
			}
			return entry;
		};

		inline void destroy(const Name& name) {
			T* res = nullptr;
			bool passToOhterHandleFunc = false;
			UserDeletor  deletor = nullptr;
			do{
				std::lock_guard<std::mutex> lock(m_mutex);
				auto it = m_entries.find(name);
				if (it == m_entries.end()) return;
				ResourceEntry* entry = it->second.get();
				if (entry->lifeTimeCtrl != ResourceLifetime::Manual) {
					passToOhterHandleFunc = true;
					assert(0);
					break;
				}
				res = (T*)entry->resource;
				deletor = entry->deletor;
				m_entries.erase(it);
			} while (0);
			
			if (passToOhterHandleFunc) {
				release(name);
			}
			else {

				if (deletor != nullptr) {
					deletor(name, res);
				}
				else {
					unloadImpl(res);
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
			auto entry = create(name, ResourceLifetime::Transient);

			return entry;
		}

		inline void release(const Name& id) override {
			T* res = nullptr;
			bool needDelete = false;
			UserDeletor deletor = nullptr;
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				auto it = m_entries.find(id);
				if (it == m_entries.end()) return;
				ResourceEntry* entry = it->second.get();
				if (entry->refCount == 0) return;
				res = (T*)entry->resource;
				deletor = entry->deletor;
				if (entry->refCount.fetch_sub(1) == 0) {
					if (entry->lifeTimeCtrl == ResourceLifetime::Transient) {
						m_entries.erase(it);
						needDelete = true;
					}
					else {
						//For Persistent or Manual
						//Do nothing
					}
				}
			}
			if (needDelete) {
				if (deletor) {
					deletor(id, res);
				}
				else {
					unloadImpl(res);
				}
			}

		}

		inline size_t debugRefCount(const Name& id) const {
			auto it = m_entries.find(id);
			return (it == m_entries.end()) ? 0 : it->second->refCount.load();
		}
		inline virtual void createNecessaryPersistenceResources() {};
	protected:
		virtual T* loadImpl(const Name& id) = 0;

		virtual void unloadImpl(T* /*resource*/) = 0;

	private:
		std::mutex m_mutex;
		std::map<Name, std::unique_ptr<ResourceEntry>> m_entries;
	};

}

#endif