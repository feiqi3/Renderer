#include "common/ResourceSystem.h"
#include "common/ResourceManager.h"
namespace Render {

	void ResourceSystem::registerSystem(std::unique_ptr<IResourceManager> manager)
	{
		const auto& name = manager->typeName();
		auto itor = mManagers.find(name);
		if (itor != mManagers.end()) {
			return;
		}
		mManagers.insert({ name,std::move(manager) });
	}

	void ResourceSystem::unregisterSystem(const Name& resourceType)
	{
		auto itor = mManagers.find(resourceType);
		if (itor != mManagers.end()) {
			mManagers.erase(resourceType);
		}
	}

	Render::IResourceManager* ResourceSystem::getResourceManager(const Name& name)
	{
		auto itor = mManagers.find(name);
		if (itor != mManagers.end()) {
			return itor->second.get();
		}
		return nullptr;
	}

	bool ResourceSystem::isResourceManagerExist(const Name& name) const
	{
		auto itor = mManagers.find(name);
		if (itor != mManagers.end()) {
			return true;
		}
		return false;
	}

}
