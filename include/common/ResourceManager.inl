#include "ResourceManager.h"
#ifndef _RESOURCE_MANAGER_INL_
#define _RESOURCE_MANAGER_INL_


namespace Render {
	const static Name anonymousName = Name("_anonymous");

	template<typename T>
	ResourceManager<T>::ResourceManager() {}

	template<typename T>
	ResourceEntry* ResourceManager<T>::registerAnonymousResource(IResource* resource, ResourceLifetime lifeTimeCtr, UserDeletor deletor)
	{
		const Name& tarName = resource->getTypeName();
		ResourceEntry* ret = nullptr;
		if (tarName != this->typeName()) {
			assert(0);
			throw std::runtime_error("Resource Type Error");
		}
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			auto it = m_managedEntries.find(resource);
			if (it != m_managedEntries.end()) {
				it->second->refCount++;
				return it->second.get();
			}
			auto entry = std::make_unique<ResourceEntry>();
			entry->resourceName = anonymousName;
			entry->anonymousResource = true;
			entry->resource = resource;
			entry->refCount = 1;
			entry->lifeTimeCtrl = lifeTimeCtr;
			entry->deletor = deletor;
			ret = entry.get();
			this->m_managedEntries.insert({ resource, std::move(entry)});
		}
		if (ret) {
			ret->resource->OnLoaded();
		}
		return ret;
	}

	template<typename T>
	ResourceEntry* ResourceManager<T>::registerResource(const Name& id, IResource* resource, ResourceLifetime lifeTimeCtr, UserDeletor deletor) {
		const Name& tarName = resource->getTypeName();
		ResourceEntry* ret = nullptr;
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
			ret = entry.get();
			this->m_entries.insert({ id, std::move(entry) });
		}
		if (ret) {
			ret->resource->OnLoaded();
		}
		return ret;
	}

	template<typename T>
	ResourceEntry* ResourceManager<T>::acquire(const Name& name) {
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

	template<typename T>
	ResourceEntry* ResourceManager<T>::create(const Name& name, ResourceLifetime lifeTimeCtr) {
		T* res = loadImpl(name);
		if (!res) return nullptr;
		bool needRelease = false;

		ResourceEntry* entry = nullptr;
		{
			// Case when loaded in other thread or loaded repeatedly
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
				m_entries.insert({ name, std::move(e) });
			}
		}

		if (needRelease) {
			unloadImpl(res);
		}
		else {
			res->OnLoaded();
		}
		return entry;
	}

	template<typename T>
	void ResourceManager<T>::destroy(const Name& name) {
		T* res = nullptr;
		bool passToOtherHandleFunc = false;
		UserDeletor deletor = nullptr;
		do {
			std::lock_guard<std::mutex> lock(m_mutex);
			auto it = m_entries.find(name);
			if (it == m_entries.end()) return;
			ResourceEntry* entry = it->second.get();
			if (entry->lifeTimeCtrl != ResourceLifetime::Manual) {
				passToOtherHandleFunc = true;
				assert(0);
				break;
			}
			res = (T*)entry->resource;
			deletor = entry->deletor;
			m_entries.erase(it);
		} while (0);

		if (passToOtherHandleFunc) {
			release(name);
		}
		else {
			res->OnUnload();
			if (deletor != nullptr) {
				deletor(name, res);
			}
			else {
				unloadImpl(res);
			}
		}
	}

	template<typename T>
	ResourceEntry* ResourceManager<T>::acquireOrCreate(const Name& name) {
		{
			ResourceEntry* resource = acquire(name);
			if (resource != nullptr) {
				return resource;
			}
		}
		auto entry = create(name, ResourceLifetime::Transient);
		return entry;
	}

	template<typename T>
	void ResourceManager<T>::release(const Name& id) {
		T* res = nullptr;
		bool needDelete = false;
		UserDeletor deletor = nullptr;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			auto it = m_entries.find(id);
			if (it == m_entries.end()) return;
			ResourceEntry* entry = it->second.get();

			// refCount is atomic, but we are inside a lock anyway which protects the map entry existence
			// However, since we might erase it, lock is necessary.

			if (entry->refCount == 0) return;

			res = (T*)entry->resource;
			deletor = entry->deletor;

			if (entry->refCount.fetch_sub(1) == 1) { // fetch_sub returns OLD value. So if old was 1, new is 0.
				if (entry->lifeTimeCtrl == ResourceLifetime::Transient) {
					m_entries.erase(it);
					needDelete = true;
				}
				else {
					// For Persistent or Manual -> Do nothing
				}
			}
		}
		if (needDelete) {
			res->OnUnload();
			if (deletor) {
				deletor(id, res);
			}
			else {
				unloadImpl(res);
			}
		}
	}


	template<typename T>
	void Render::ResourceManager<T>::releaseAnonymous(IResource* resource)
	{
		T* res = nullptr;
		bool needDelete = false;
		UserDeletor deletor = nullptr;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			auto it = m_managedEntries.find(resource);
			if (it == m_managedEntries.end()) return;
			ResourceEntry* entry = it->second.get();

			// refCount is atomic, but we are inside a lock anyway which protects the map entry existence
			// However, since we might erase it, lock is necessary.

			if (entry->refCount == 0) return;

			res = (T*)entry->resource;
			deletor = entry->deletor;

			if (entry->refCount.fetch_sub(1) == 1) { // fetch_sub returns OLD value. So if old was 1, new is 0.
				if (entry->lifeTimeCtrl == ResourceLifetime::Transient) {
					m_managedEntries.erase(it);
					needDelete = true;
				}
				else {
					// For Persistent or Manual -> Do nothing
				}
			}
		}
		if (needDelete) {
			res->OnUnload();
			if (deletor) {
				deletor(anonymousName, res);
			}
			else {
				unloadImpl(res);
			}
		}
	}


	template<typename T>
	inline const Name& ResourceManager<T>::getDefaultResourceName() const
	{
		return Name::_emptyName;
	}

	template<typename T>
	inline void ResourceManager<T>::clearAll()
	{
		std::vector<std::unique_ptr<ResourceEntry>> resourcesToUnload;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			for (auto& [name, entryPtr] : m_entries) {
				auto res = entryPtr->resource;
				resourcesToUnload.push_back(std::move(entryPtr));
			}
			m_entries.clear();

			for (auto& [resource, entryPtr] : m_managedEntries) {
				auto res = entryPtr->resource;
				resourcesToUnload.push_back(std::move(entryPtr));
			}
			m_managedEntries.clear();

		}
		for (int i = 0; i < resourcesToUnload.size(); ++i) {
			auto& entry = resourcesToUnload[i];
			auto res = entry->resource;
			res->OnUnload();
			if (entry->deletor) {
				entry->deletor(entry->resourceName, res);
			}
			else {
				unloadImpl(static_cast<T*>(res));
			}
		}
	}

	template<typename T>
	size_t ResourceManager<T>::debugRefCount(const Name& id) const {
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_entries.find(id);
		return (it == m_entries.end()) ? 0 : it->second->refCount.load();
	}

} // namespace Render

#endif // _RESOURCE_MANAGER_INL_