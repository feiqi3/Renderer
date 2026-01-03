#include "function/EngineResourceManager.h"
#include "common/ResourceSystem.h"

#include "Renderer/TextureResourceMgr.h"
namespace Render {

	std::vector<Name> sRegisteredResourceName{};

	void RegisterAllEngineResourceManager() {
		auto resSystem = ResourceSystem::instance();
		{
			auto TextureManager = std::make_unique<TextureResourceManager>();
			sRegisteredResourceName.push_back(TextureManager->typeName());
			resSystem->registerSystem(std::move(TextureManager));
		}
	}
	
	void UnRegisterAllEngineResourceManager() {
		auto resSystem = ResourceSystem::instance();
		for (const auto& name : sRegisteredResourceName) {
			resSystem->unregisterSystem(name);
		}
	}
}