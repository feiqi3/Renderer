#include "function/EngineResourceManager.h"
#include "common/ResourceSystem.h"
#include "Renderer/MaterialTemplateManager.h"
#include "Renderer/TextureResourceMgr.h"
#include "Renderer/MeshResourceManager.h"
#include "Renderer/MaterialManager.h"
#include "Renderer/SamplerResourceManager.h"
#include "Renderer/SkeletonResourceManager.h"
#include "Renderer/AnimationResourceManager.h"
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

			auto MatManager = std::make_unique<MaterialManager>();
			sRegisteredResourceName.push_back(MatManager->typeName());
			resSystem->registerSystem(std::move(MatManager));
			
			auto SamplerManager = std::make_unique<SamplerResourceManager>();
			sRegisteredResourceName.push_back(SamplerManager->typeName());
			resSystem->registerSystem(std::move(SamplerManager));

			auto skeletonResourceManager = std::make_unique<SkeletonResourceManager>();
			sRegisteredResourceName.push_back(skeletonResourceManager->typeName());
			resSystem->registerSystem(std::move(skeletonResourceManager));

			auto skeletonAnmResourceManager = std::make_unique<SkeletonAnimationResourceManager>();
			sRegisteredResourceName.push_back(skeletonAnmResourceManager->typeName());
			resSystem->registerSystem(std::move(skeletonAnmResourceManager));
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
			resSystem->clearSystem(name);
		}

		for (const auto& name : sRegisteredResourceName) {
			resSystem->unregisterSystem(name);
		}
	}
}