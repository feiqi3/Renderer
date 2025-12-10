#ifndef RESOURCE_SYSTEM_H_
#define RESOURCE_SYSTEM_H_
#include "common/Singleton.h"
#include "common/Name.h"
#include "common/ResourceManager.h"
#include <vector>
#include <memory>
namespace Render {
	class IResourceManager;
	class ResourceSystem : public Singleton< ResourceSystem> {
	public:
		void registerSystem(std::unique_ptr<IResourceManager> manager);
		void unregisterSystem(Name& resourceType);
		IResourceManager* getResourceManager(const Name& name);
		
		template<typename T>
		inline ResourceHandle<T> getResource(const Name& type, const Name& resource) {
			auto mgr = getResourceManager(type);
			return ResourceHandle<T>(mgr,mgr->acquire(resource));
		}
		bool isResourceManagerExist(Name& name)const;
	private:
		std::vector< std::unique_ptr<IResourceManager>> mManagers;
	};
}

#endif