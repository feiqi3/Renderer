#include "common/ResourceSystem.h"
#include "common/ResourceManager.h"
namespace Render {

	void ResourceSystem::registerSystem(std::unique_ptr<IResourceManager> manager)
	{
		for (auto&& sys : mManagers) {
			if (sys->typeName() == manager->typeName() ) {
				throw std::runtime_error("Duplicated manager.");
				return;
			}
		}
		mManagers.push_back(std::move(manager));
	}

	void ResourceSystem::unregisterSystem(const Name& resourceType)
	{
		int pos = -1;
		for (int i = 0;i < mManagers.size();i++) {
			if (mManagers[i]->typeName() == resourceType) {
				pos = i;
				break;
			}
		}
		if (pos == -1) {
			return;
		}
		std::swap(mManagers[pos], mManagers[mManagers.size() - 1]);
		mManagers.pop_back();
	}

	Render::IResourceManager* ResourceSystem::getResourceManager(const Name& name)
	{
		for (auto&& resourceMgr : mManagers) {
			if (resourceMgr->typeName() == name) {
				return resourceMgr.get();
			}
		}
		return nullptr;
	}

	bool ResourceSystem::isResourceManagerExist(const Name& name) const
	{
		for (auto&& resourceMgr : mManagers) {
			if (resourceMgr->typeName() == name) {
				return true;
			}
		}
		return false;
	}

}
