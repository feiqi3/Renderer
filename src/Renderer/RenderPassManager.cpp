#include "Renderer/RenderSystem.h"
#include "Renderer/RenderPassManager.h"
#include "Renderer/RenderPass/VirtualRenderPass.h"
#include "vulkan/vulkan_pipeline.h"
namespace Render {

	RenderPass* RenderPassManager::getRenderPass(const Name& passName)
	{
		auto itor = mRenderPasses.find(passName);
		if (itor != mRenderPasses.end()) {
			return itor->second;
		}
		return nullptr;
	}

	std::vector<Name> RenderPassManager::getAllRenderPassNames() const
	{
		std::vector<Name> passNames;
		for (auto& [name, pass] : mRenderPasses) {
			passNames.push_back(name);
		}
		return passNames;
	}

	void RenderPassManager::removeRenderPass(const Name& name)
	{
		auto pass = getRenderPass(name);
		if (pass) {
			mRenderPasses.erase(name);
		}
	}

	RenderPassManager::~RenderPassManager()
	{
		mVirtualRenderPass = 0;

		for (auto& [name, pass] : mRenderPasses) {
			destroyRenderPass(pass);
		}
		mRenderPasses.clear();

	}

	RenderPassManager::RenderPassManager()
	{
		mVirtualRenderPass = new VirtualRenderPass();
		this->registerRenderPass(mVirtualRenderPass);
	}

	void RenderPassManager::onFrameBegin()
	{

		//If target renderpass exists   
		//Try create a renderpass for it
		auto sys = RenderSystem::instance();
		uint32_t TargetSwapchainImg = sys->getNextRenderFrame() % sys->getRenderContext()->maxSwapChainImages;
	}

	void RenderPassManager::registerRenderPass(RenderPass* pass)
	{
		for (const auto& logicalPass : pass->getLogicalPasses()) {
			addRenderPass(logicalPass.name, pass);
		}
	}

	void RenderPassManager::unregisterRenderPass(RenderPass* pass)
	{
		for (const auto& logicalPass : pass->getLogicalPasses()) {
			removeRenderPass(logicalPass.name);
		}
	}

	void RenderPassManager::addRenderPass(const Name& passName, RenderPass* pass)
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
		if (pass->mRenderPass) {
			RenderSystem::instance()->destoyRenderPass(pass->mRenderPass);
			pass->mRenderPass = nullptr;
		}
		delete pass;
	}
}
