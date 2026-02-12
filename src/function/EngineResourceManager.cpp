#include "function/EngineResourceManager.h"
#include "common/ResourceSystem.h"
#include "Renderer/MaterialTemplateManager.h"
#include "Renderer/TextureResourceMgr.h"
#include "Renderer/MeshResourceManager.h"
namespace Render {

	std::vector<Name> sRegisteredResourceName{};

	void RegisterAllEngineResourceManager() {
		auto resSystem = ResourceSystem::instance();
		{
			auto TextureManager = std::make_unique<TextureResourceManager>();
			sRegisteredResourceName.push_back(TextureManager->typeName());
			resSystem->registerSystem(std::move(TextureManager));
			auto MeshManager = std::make_unique<MeshResourceManager>();
			sRegisteredResourceName.push_back(MeshManager->typeName());
			resSystem->registerSystem(std::move(MeshManager));
			auto MatTempManager = std::make_unique<MaterialTemplateManager>();
			sRegisteredResourceName.push_back(MatTempManager->typeName());
			resSystem->registerSystem(std::move(MatTempManager));
		}
	}

	void CreateAllPersistentResource()
	{
		for (const auto& name : sRegisteredResourceName) {
			auto resSystem = ResourceSystem::instance();
			auto resMgr = resSystem->getResourceManager(name);
			resMgr->createNecessaryPersistenceResources();
		}
	}
	
	void UnRegisterAllEngineResourceManager() {
		auto resSystem = ResourceSystem::instance();
		for (const auto& name : sRegisteredResourceName) {
			resSystem->unregisterSystem(name);
		}
	}
}