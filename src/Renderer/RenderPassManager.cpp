#include "Renderer/RenderSystem.h"
#include "Renderer/RenderPassManager.h"
#include "vulkan/vulkan_pipeline.h"
namespace Render {
	RenderPass* RenderPassManager::getRenderPass(const std::string& passName)
	{
		auto itor = mRenderPasses.find(passName);
		if (itor != mRenderPasses.end()) {
			return itor->second;
		}
		return nullptr;
	}

	void RenderPassManager::removeRenderPass(const std::string& name)
	{
		auto pass = getRenderPass(name);
		if (pass) {
			mRenderPasses.erase(name);
		}
	}

	RenderPassManager::~RenderPassManager()
	{
		for (auto& [name, pass] : mRenderPasses) {
			destroyRenderPass(pass);
		}
		mRenderPasses.clear();
	}

	void RenderPassManager::addRenderPass(const std::string& passName, RenderPass* pass)
	{
		auto itor = mRenderPasses.find(passName);
		if (itor == mRenderPasses.end()) {
			mRenderPasses.insert({ passName,pass });
		}
		else {
		}
	}

	void RenderPassManager::destroyRenderPass(RenderPass* pass)
	{
		RenderSystem::instance()->destoyRenderPass(pass->mRenderPass);
		delete pass;
	}
}
