#include "Renderer/RenderSystem.h"
#include "Renderer/RenderPassManager.h"
#include "vulkan/vulkan_pipeline.h"
namespace Render {

	class SwapchainRenderPassHelper {
	public:  
		std::vector<rs_renderpass*> swapchainRenderPass;
		std::string RenderPassName;
		RenderPass* TargetRenderPass = 0;
		void clearRenderPass() {
			for (auto&& rp : swapchainRenderPass) {
				RenderSystem::instance()->destoyRenderPass(rp);
				rp = 0;
			}
		}

		rs_renderpass* getCurframeSwapchainRenderPass(const PassDesc& desc,uint32_t curSwapchainImage) {
			auto sys = RenderSystem::instance();
			if (swapchainRenderPass.size() < curSwapchainImage + 1 ||  swapchainRenderPass[curSwapchainImage] == nullptr) {
				swapchainRenderPass.resize(curSwapchainImage + 1);
				swapchainRenderPass[curSwapchainImage] = sys->createRenderPass(sys->getNextSwapchainRendertarget(), desc);
			}
			return swapchainRenderPass[curSwapchainImage];
		}

		void rebuildAll() {
			clearRenderPass();
			auto sys = RenderSystem::instance();
			swapchainRenderPass.resize(sys->getRenderContext()->maxSwapChainImages, nullptr);
		}

		~SwapchainRenderPassHelper() {
			TargetRenderPass = 0;
			RenderPassName = "";
			clearRenderPass();
			swapchainRenderPass.clear();
		}
	};


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
		delete mSwapchainRpHelper;
	}

	RenderPassManager::RenderPassManager()
	{
		mSwapchainRpHelper = new SwapchainRenderPassHelper;
	}

	void RenderPassManager::onFrameBegin()
	{

		if (!mSwapchainRpHelper->TargetRenderPass) {
			return;
		}	

		//If target renderpass exists   
		//Try create a renderpass for it
		auto targettRp = mSwapchainRpHelper->TargetRenderPass;
		auto sys = RenderSystem::instance();
		uint32_t TargetSwapchainImg = sys->getNextRenderFrame() % sys->getRenderContext()->maxSwapChainImages;
		targettRp->mRenderPass = mSwapchainRpHelper->getCurframeSwapchainRenderPass(targettRp->mPassDesc, TargetSwapchainImg);
	}

	void RenderPassManager::markSwapchainRenderPass(RenderPass* pass)
	{
		mSwapchainRpHelper->RenderPassName = pass->getPassName();
		mSwapchainRpHelper->TargetRenderPass = pass;

		if (mSwapchainRpHelper->TargetRenderPass->mRenderPass) {
			RenderSystem::instance()->destoyRenderPass(mSwapchainRpHelper->TargetRenderPass->mRenderPass);
			mSwapchainRpHelper->TargetRenderPass->mRenderPass = nullptr;
		}

		mSwapchainRpHelper->clearRenderPass();
		auto sys = RenderSystem::instance();
		uint32_t TargetSwapchainImg = sys->getNextRenderFrame() % sys->getRenderContext()->maxSwapChainImages;
		pass->mRenderPass = mSwapchainRpHelper->getCurframeSwapchainRenderPass(pass->mPassDesc, TargetSwapchainImg);
	}

	void RenderPassManager::unMarkSwapchainRenderPass(RenderPass* pass)
	{
		if (pass != mSwapchainRpHelper->TargetRenderPass) {
			return;
		}
		mSwapchainRpHelper->clearRenderPass();
		mSwapchainRpHelper->RenderPassName = "";
		mSwapchainRpHelper->TargetRenderPass->mRenderPass = nullptr;
		mSwapchainRpHelper->TargetRenderPass = nullptr;
	}

	void RenderPassManager::onSwapchainRebuild()
	{
		this->mSwapchainRpHelper->rebuildAll();
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
		if (pass == mSwapchainRpHelper->TargetRenderPass) {
			unMarkSwapchainRenderPass(pass);
		}
		if (pass->mRenderPass) {
			RenderSystem::instance()->destoyRenderPass(pass->mRenderPass);
		}
		delete pass;
	}
}
