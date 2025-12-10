#ifndef _RESOURCE_MANAGER_H_
#define _RESOURCE_MANAGER_H_

#include "common/CoreDefs.h"
#include "common/Name.h"
#include "common/UUID.h"
#include <string>
#include <memory>
namespace Render{

	struct ResourceEntry {
		Name resourceName;
		void* resource;
		size_t refCount = 0;
	};

	class IResourceManager {
	public:
		virtual ~IResourceManager() = default;
		virtual const Name& typeName() const = 0;

		virtual ResourceEntry* acquire(const Name& id) = 0;
		virtual void release(const Name& id) = 0;
	};


	template<typename T>
	class ResourceManager : public IResourceManager {
	public:


		explicit ResourceManager(Name& typeName) : m_typeName(typeName) {}
		virtual ~ResourceManager() = default;

		const Name& typeName() const override { return m_typeName; }

		ResourceEntry* acquire(const Name& name) override {
			auto it = m_entries.find(name);
			if (it == m_entries.end()) {
				T* res = loadImpl(name);
				if (!res) return nullptr;
				auto e = new unique_ptr<ResourceEntry>();
				e->resourceName = name;
				e->resource = (res);
				e->refCount = 1;
				auto raw = e->get();
				m_entries.emplace(name, std::move(e));
				return raw;
			}
			else {
				it->second->refCount++;
				return it->second.get();
			}
		}

		void release(const Name& id) override {
			auto it = m_entries.find(id);
			if (it == m_entries.end()) return;
			if (it->second.refCount == 0) return; 
			it->second.refCount--;
			if (it->second.refCount == 0) {
				unloadImpl(it->second.resource);
				m_entries.erase(it);
			}
		}

		size_t debugRefCount(const Name& id) const {
			auto it = m_entries.find(id);
			return (it == m_entries.end()) ? 0 : it->second.refCount;
		}

	protected:
		virtual T* loadImpl(const Name& id) = 0;

		virtual void unloadImpl(T* /*resource*/) {  }

	private:
		std::map<Name, std::unique_ptr<ResourceEntry>> m_entries;
		Name m_typeName;
	};

}

#endif