#ifndef RESOURCE_SYSTEM_H_
#define RESOURCE_SYSTEM_H_
#include "common/Singleton.h"
#include "common/Name.h"
#include "common/ResourceHandler.h"
#include <vector>
#include <memory>
namespace Render {
	class IResourceManager;
	class ResourceSystem : public Singleton<ResourceSystem> {
	public:
		void registerSystem(std::unique_ptr<IResourceManager> manager);
		void unregisterSystem(const Name& resourceType);
		IResourceManager* getResourceManager(const Name& name);

		template<typename T>
		inline ResourceHandle<T> createResource(const Name& type, const Name& resource,ResourceLifetime lifeTime = ResourceLifetime::Transient) {
			auto mgr = getResourceManager(type);
			if (!mgr)return nullptr;
			return ResourceHandle<T>(mgr, mgr->create(resource,lifeTime));
		}

		template<typename T>
		inline ResourceHandle<T> registerResource(const Name& type, const Name& resource,T* pRes, ResourceLifetime lifeTime = ResourceLifetime::Transient,UserDeletor deletor = nullptr) {
			auto mgr = getResourceManager(type);
			if (!mgr)return nullptr;
			return ResourceHandle<T>(mgr, mgr->registerResource(resource, pRes, lifeTime,deletor));
		}

		inline void destroyResource(const Name& type, const Name& resource) {
			auto mgr = getResourceManager(type);
			if (!mgr)return;
			mgr->destroy(resource);
		}

		template<typename T> 
		inline ResourceHandle<T> getOrCreateResource(const Name& type, const Name& resource) {
			auto mgr = getResourceManager(type);
			if (!mgr)return nullptr;
			return ResourceHandle<T>(mgr, mgr->acquireOrCreate(resource));
		}
		template<typename T>
		inline ResourceHandle<T> getResource(const Name& type, const Name& resource) {
			auto mgr = getResourceManager(type);
			if (!mgr)return nullptr;
			ResourceManager<T>* mgrCasted = dynamic_cast<ResourceManager<T>*>(mgr);
			assert(mgrCasted != nullptr && "Wrong resource type");
			if (!mgrCasted) {
				return nullptr;
			}
			return ResourceHandle<T>(mgr, mgr->acquire(resource));
		}
		bool isResourceManagerExist(const Name& name)const;
	private:
		std::map<Name, std::unique_ptr<IResourceManager>> mManagers;
		u32 mResourceTypeCnt = 0;
	};

#include "function/EngineResourceNameList.inl"

}

#endif