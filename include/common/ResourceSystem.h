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
		inline ResourceHandle<T> getOrCreateResource(const Name& type, const Name& resource, void* userCreateInfo) {
			auto mgr = getResourceManager(type);
			if (!mgr)return nullptr;
			return ResourceHandle<T>(mgr, mgr->acquire(resource), userCreateInfo);
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
		std::vector< std::unique_ptr<IResourceManager>> mManagers;
	};
}

#endif