#include "common/ResourceSystem.h"
#include "common/ResourceManager.h"
namespace Render {

	void ResourceSystem::registerSystem(std::unique_ptr<IResourceManager> manager)
	{
		mManagers.push_back(std::move(manager));
	}

	void ResourceSystem::unregisterSystem(Name& resourceType)
	{
		int pos = -1;
		for (int i = 0;i < mManagers.size();i++) {
			if (mManagers[i]->typeName() == resourceType) {
				pos = i;
			}
		}
		if (pos == -1) {
			return;
		}
		std::swap(mManagers[pos], mManagers[mManagers.size() - 1]);
		mManagers.pop_back();
	}

	Render::IResourceManager* ResourceSystem::getResourceManager(Name& name)
	{
		for (auto&& resourceMgr : mManagers) {
			if (resourceMgr->typeName() == name) {
				return resourceMgr.get();
			}
		}
		return nullptr;
	}

	bool ResourceSystem::isResourceManagerExist(Name& name) const
	{
		for (auto&& resourceMgr : mManagers) {
			if (resourceMgr->typeName() == name) {
				return true;
			}
		}
		return false;
	}

}
