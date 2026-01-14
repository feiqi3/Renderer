#ifndef RENDER_FLOW_H
#define RENDER_FLOW_H
#include "Renderer/RenderPass/MainCameraPass.h"
#include "SwapchainPass.h"   
#include "RenderSystem.h"
#include "RenderPassManager.h"
#include "RenderQueue.h"
#include "Texture.h"
namespace Render {
	class RenderFlowBase {
	public:
		RenderFlowBase() = default;
		virtual ~RenderFlowBase() = default;
	public:

		struct RenderTargetPack {
			Name renderTargetName;
			rs_rendertarget* renderTarget;
			std::vector<TexturePtr> textures;
		};

	protected:
		
		std::list<RenderTargetPack> mRenderTargets;

	};

	class RenderFlow {
	public:

		void init() {
			mMainCamPass = new MainCameraPass;
			mSwapchainPass = new SwapchainPass;
			initMainCamPass();
			initSwapChainPass();
			this->mOffscreenFinishSemaphore = RenderSystem::instance()->createSemphore();
			this->mAccquireImgSemaphore = RenderSystem::instance()->createSemphore();
			this->mPresentToScreenSemaphore = RenderSystem::instance()->createSemphore();
			mWaitForRenderEndFence = RenderSystem::instance()->createFence();
			auto RenderSys = RenderSystem::instance();
			RenderSys->setRenderFence(mWaitForRenderEndFence);
			//Is Present image accquired?  
			RenderSys->setSignalCanPresentToPresentImageSemaphore(mPresentToScreenSemaphore);
			//Is Rendered to present image finished?
			RenderSys->setSignalCanRenderToPresentImageSemaphore(mAccquireImgSemaphore);

		}

		void initMainCamPass() {
			mMainCamPass->init();
			auto rsys = RenderSystem::instance();
			rsys->getRenderPassManager()->registerRenderPass(mMainCamPass);
			mCol = rsys->createRTTexture(RenderTextureFormat::RGBA8, 1000, 1000, 1, 1, true);
			mDepth = rsys->createDepthStencilTexture(RenderTextureFormat::D24S8, 1000, 1000, false);
			mRenderTarget = rsys->createRendertarget({ mCol }, mDepth);
			mMainCamPass->setRenderTarget(mRenderTarget);
		}

		void initSwapChainPass() {
			RenderSystem* renderSys = RenderSystem::instance();
			mSwapchainPass->init();
			renderSys->getRenderPassManager()->registerRenderPass(mSwapchainPass);
		};
		void deinitSwapchainPass() {
			RenderSystem* renderSys = RenderSystem::instance();
			renderSys->getRenderPassManager()->unregisterRenderPass(mSwapchainPass);
			delete mSwapchainPass;
		}

		void deinitMainCamPass() {
			RenderSystem* renderSys = RenderSystem::instance();
			renderSys->getRenderPassManager()->unregisterRenderPass(mMainCamPass);
			delete mMainCamPass;
			mMainCamPass = 0;
		}

		void deinit() {
			deinitMainCamPass();
			deinitSwapchainPass();

			RenderSystem::instance()->destroySemphore(mOffscreenFinishSemaphore);
			RenderSystem::instance()->destroySemphore(mAccquireImgSemaphore);
			RenderSystem::instance()->destroySemphore(mPresentToScreenSemaphore);
			RenderSystem::instance()->destroyFence(mWaitForRenderEndFence);
		}

		void AddEntity(RenderEntity* entity) {
			mRenderEntities.push_back(entity);
		}

		void Excute(){
			auto RenderSys = RenderSystem::instance();
			auto cmdbufOffscreen = RenderSys->GetCommandBufferCurFrameCurThread();
			RenderSys->cmdBegin(cmdbufOffscreen);
			mMainCamPass->draw(cmdbufOffscreen);
			RenderSys->cmdEnd(cmdbufOffscreen);
			RenderSys->submitCmdBuffer(cmdbufOffscreen, {}, { mOffscreenFinishSemaphore }, nullptr);

			auto cmdbufSwapchain = RenderSys->GetCommandBufferCurFrameCurThread();

			RenderSys->cmdBegin(cmdbufSwapchain);
			mSwapchainPass->setBlitRT(mCol);
			mSwapchainPass->draw(cmdbufSwapchain);
			RenderSys->cmdEnd(cmdbufSwapchain);
			RenderSys->submitCmdBuffer(cmdbufSwapchain, { mOffscreenFinishSemaphore ,mAccquireImgSemaphore}, { mPresentToScreenSemaphore }, mWaitForRenderEndFence);
			RenderSys->getMainRenderQueue()->clear();
		}
	private:
		MainCameraPass* mMainCamPass = 0;
		SwapchainPass* mSwapchainPass = 0;
		std::vector<RenderEntity*> mRenderEntities;
		std::vector<RenderEntity*> mPostEffectEntities;
		
		rs_semaphore* mOffscreenFinishSemaphore;
		rs_semaphore* mAccquireImgSemaphore;
		rs_semaphore* mPresentToScreenSemaphore;
		rs_fence* mWaitForRenderEndFence;
		RenderEntity* BlitEntity;
		MaterialTemplate* BlitMaterial;

		rs_image* mCol = nullptr;
		rs_image* mDepth = nullptr;
		rs_rendertarget* mRenderTarget = nullptr;
	};
}

#endif