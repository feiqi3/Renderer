#ifndef RENDER_FLOW_H
#define RENDER_FLOW_H
#include "MainCameraRenderPass.h"
#include "SwapchainPass.h"   
#include "RenderSystem.h"

namespace Render {
	class RenderFlow {
	public:

		void init() {
			mMainCamPass = new MainCamPass;
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
			mMainCamRenderTexture = RenderSystem::instance()->createRTTexture(Render::ImageFormat::BGRA8_UNORM, 1920, 1080, 1, 1, true);
			mRenderTarget = RenderSystem::instance()->createRendertarget({ mMainCamRenderTexture }, 0);
			mMainCamPass->setRenderTarget(mRenderTarget);
			mMainCamPass->init();
		}

		void initSwapChainPass() {
			RenderSystem* renderSys = RenderSystem::instance();
			mSwapchainPass->init();
		};
		void deinitSwapchainPass() {
			delete mSwapchainPass;
		}

		void deinitMainCamPass() {
			delete mMainCamPass;
			mMainCamPass = 0;

			delete mRenderTarget;
			mRenderTarget = 0;

			delete mMainCamRenderTexture;
			mMainCamRenderTexture = 0;
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
			for (auto&& entity : mRenderEntities) {
				mMainCamPass->addToDraw(entity);
			}
			mMainCamPass->draw(cmdbufOffscreen);
			RenderSys->cmdEnd(cmdbufOffscreen);
			RenderSys->submitCmdBuffer(cmdbufOffscreen, {}, { mOffscreenFinishSemaphore }, nullptr);

			auto cmdbufSwapchain = RenderSys->GetCommandBufferCurFrameCurThread();

			RenderSys->cmdBegin(cmdbufSwapchain);
			mSwapchainPass->setBlitRT(mMainCamRenderTexture);
			mSwapchainPass->draw(cmdbufSwapchain);
			RenderSys->cmdEnd(cmdbufSwapchain);
			RenderSys->submitCmdBuffer(cmdbufSwapchain, { mOffscreenFinishSemaphore ,mAccquireImgSemaphore}, { mPresentToScreenSemaphore }, mWaitForRenderEndFence);

		}
	private:
		MainCamPass* mMainCamPass = 0;
		rs_image* mMainCamRenderTexture = 0;
		rs_rendertarget* mRenderTarget = 0;
		SwapchainPass* mSwapchainPass = 0;
		std::vector<RenderEntity*> mRenderEntities;
		std::vector<RenderEntity*> mPostEffectEntities;
		
		rs_semaphore* mOffscreenFinishSemaphore;
		rs_semaphore* mAccquireImgSemaphore;
		rs_semaphore* mPresentToScreenSemaphore;
		rs_fence* mWaitForRenderEndFence;
		RenderEntity* BlitEntity;
		MaterialTemplate* BlitMaterial;
	};
}

#endif