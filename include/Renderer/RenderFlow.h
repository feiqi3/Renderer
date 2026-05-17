#ifndef RENDER_FLOW_H
#define RENDER_FLOW_H
#include "Renderer/RenderPass/MainCameraPass.h"
#include "SwapchainPass.h"   
#include "RenderSystem.h"
#include "RenderPassManager.h"
#include "RenderQueue.h"
#include "Texture.h"
#include "function/Scene.h"
#include "Renderer/Camera.h"
#include "Renderer/CameraManager.h"
#include "Renderer/LightManager.h"
#include "Renderer/RenderPass/PostEffectComposePass.h"
#include "Renderer/PostEffect/CODBloom.h"
#include "Renderer/TextureResourceMgr.h"
#include "Renderer/Blit.h"
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
			mPostEffectPass = new PostEffectComposePass();

			initMainCamPass();
			initSwapChainPass();
			initPostEffectPass();

			this->mOffscreenFinishSemaphore = RenderSystem::instance()->createSemaphore();
			this->mAccquireImgSemaphore = RenderSystem::instance()->createSemaphore();
			this->mPresentToScreenSemaphore = RenderSystem::instance()->createSemaphore();
			mWaitForRenderEndFence = RenderSystem::instance()->createFence();
			auto RenderSys = RenderSystem::instance();
			//Is Present image accquired?  
			RenderSys->setSignalCanPresentToPresentImageSemaphore(mPresentToScreenSemaphore);
			//Is Rendered to present image finished?
			RenderSys->setSignalCanRenderToPresentImageSemaphore(mAccquireImgSemaphore);
			mCamera = new Camera(Name("Scene"));
			CameraManager::instance()->RegisterCamera(mCamera, 0);

			mBloom = new CodBloom();
			mBloom->setBloomRadius(1.5);

;		}

		void initPostEffectPass() {
			auto rsys = RenderSystem::instance();
			rsys->getRenderPassManager()->registerRenderPass(mPostEffectPass);
			mPostEffectPass->init();
		}

		void deinitPostEffectPass() {
			auto rsys = RenderSystem::instance();
			rsys->getRenderPassManager()->unregisterRenderPass(mPostEffectPass);
			delete mPostEffectPass;
			mPostEffectPass = nullptr;
		}

		void initMainCamPass() {
			mMainCamPass->init();
			auto rsys = RenderSystem::instance();
			rsys->getRenderPassManager()->registerRenderPass(mMainCamPass);
			auto mainColorImg = rsys->createRTTexture(RenderTextureFormat::RGBA16F, 1024, 1024, 1, 1 ,1 , true);
			mMainColorTex = TextureResourceManager::instance()->createFromRsImage(Name("MainColorTexture"),mainColorImg);
			auto mainDepthImg = rsys->createDepthStencilTexture(RenderTextureFormat::D24S8, 1024, 1024, false);
			mMainDepthTex = TextureResourceManager::instance()->createFromRsImage(Name("MainDepthTexture"), mainDepthImg);
			mRenderTarget = rsys->createRendertarget({ mainColorImg }, mainDepthImg);
			mMainCamPass->setRenderTarget(mRenderTarget);
		}

		void initSwapChainPass() {
			RenderSystem* renderSys = RenderSystem::instance();
			renderSys->getRenderPassManager()->registerRenderPass(mSwapchainPass);
			//Swapchain draw data rely on swapchain to be registered into pass manager.
			RenderSystem::instance()->setCurrentCamera(mCamera);
			mSwapchainPass->init();
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
			CameraManager::instance()->UnregisterCamera(mCamera);
			delete mCamera;
			mCamera = nullptr;
			deinitMainCamPass();
			deinitSwapchainPass();
			deinitPostEffectPass();
			RenderSystem::instance()->destroySemaphore(mOffscreenFinishSemaphore);
			RenderSystem::instance()->destroySemaphore(mAccquireImgSemaphore);
			RenderSystem::instance()->destroySemaphore(mPresentToScreenSemaphore);
			RenderSystem::instance()->destroyFence(mWaitForRenderEndFence);
		}

		void AddEntity(RenderEntity* entity) {
			mRenderEntities.push_back(entity);
		}

		void Excute(){
			auto RenderSys = RenderSystem::instance();
			RenderSys->setCurrentCamera(mCamera);
			auto cmdbufOffscreen = RenderSys->GetCommandBufferCurFrameCurThread();


			RenderSys->cmdBegin(cmdbufOffscreen);
			RenderSys->excutePendingBufferCopies(cmdbufOffscreen);
			/////////////////////////////////////////////////////
			Scene::getCurrentScene()->getLightMgr().calculateIBLData(cmdbufOffscreen);

			//Transit common data
			RenderSys->transitDrawdataResourceState(cmdbufOffscreen, PipelineType::Graphics, RenderSys->getCurCameraDrawData());
			RenderSys->transitDrawdataResourceState(cmdbufOffscreen, PipelineType::Graphics, Scene::getCurrentScene()->getSceneDrawData());


			mMainCamPass->draw(cmdbufOffscreen);
			mBloom->draw(cmdbufOffscreen, mMainColorTex);
			RenderSys->cmdEnd(cmdbufOffscreen);
			RenderSys->submitCmdBuffer(cmdbufOffscreen, {}, { mOffscreenFinishSemaphore }, nullptr);

			auto cmdbufSwapchain = RenderSys->GetCommandBufferCurFrameCurThread();
			RenderSys->setCurrentCamera(nullptr);
			RenderSys->cmdBegin(cmdbufSwapchain);

			mPostEffectPass->setBloomTex(mBloom->outBloomTex());
			mPostEffectPass->setMainRTColorTex(mMainColorTex);

			mPostEffectPass->draw(cmdbufSwapchain);
			RenderSys->cmdEnd(cmdbufSwapchain);
			RenderSys->submitCmdBuffer(cmdbufSwapchain, { mOffscreenFinishSemaphore ,mAccquireImgSemaphore}, { mPresentToScreenSemaphore }, mWaitForRenderEndFence);
			RenderSys->getMainRenderQueue()->clear();
			
			//Wait for last time frame Render finished
			//Like frame 1 is wait for frame 0 to render finished
			if(RenderSys->getNextRenderFrame() != 0)
				RenderSys->waitForFence(mWaitForRenderEndFence,RenderSys->getCurRenderFif());
			RenderSys->resetFence(mWaitForRenderEndFence, RenderSys->getCurRenderFif());
		}
	private:
		MainCameraPass* mMainCamPass = 0;
		PostEffectComposePass* mPostEffectPass = 0;
		SwapchainPass* mSwapchainPass = 0;
		std::vector<RenderEntity*> mRenderEntities;
		std::vector<RenderEntity*> mPostEffectEntities;
		
		rs_semaphore* mOffscreenFinishSemaphore;
		rs_semaphore* mAccquireImgSemaphore;
		rs_semaphore* mPresentToScreenSemaphore;
		rs_fence* mWaitForRenderEndFence;
		RenderEntity* BlitEntity;
		MaterialTemplate* BlitMaterial;

		rs_image* mainDepthImg = nullptr;
		rs_rendertarget* mRenderTarget = nullptr;
		CodBloom* mBloom = nullptr;
		Camera* mCamera = nullptr;

		TexturePtr mMainColorTex = nullptr;
		TexturePtr mMainDepthTex = nullptr;
	};
}

#endif