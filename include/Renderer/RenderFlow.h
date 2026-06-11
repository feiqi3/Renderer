#ifndef RENDER_FLOW_H
#define RENDER_FLOW_H
#include "Renderer/RenderPass/MainCameraPass.h"
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
#include "Renderer/EnginePass.h"
#include "Renderer/DebugDrawManager.h"
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
			mPostEffectPass = new PostEffectComposePass();

			initMainCamPass();
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
			mMainCamera = new Camera(Name("SceneMainCamera"));
			mMainCamera->setCullMask(CullMask::MainCamera);
			CameraManager::instance()->RegisterCamera(mMainCamera, 0);
			DebugDrawManager::instance()->init();
			mBloom = new CodBloom();
			mBloom->setBloomRadius(1.5);

;		}

		void deinitDirectionalLightShadowPass() {
			auto rsys = RenderSystem::instance();
			rsys->getRenderPassManager()->registerRenderPass(mDirectionalLightRenderPass);
			delete mDirectionalLightRenderPass;
		}

		void initDirectionalLightShadowPass() {
			auto rsys = RenderSystem::instance();
			PassDesc directionalShadowPassDesc{};
			PassAttachment attachmentDepth{};
			attachmentDepth.fmt = RenderTextureFormat::R16F;
			directionalShadowPassDesc.attachments.push_back({
				attachmentDepth
				});
			directionalShadowPassDesc.lastDepth = true;
			directionalShadowPassDesc.writeDepth = true;

			mDirectionalLightRenderPass = new RenderPass(PassName::DirectionalShadowPass, directionalShadowPassDesc);
			rsys->getRenderPassManager()->registerRenderPass(mDirectionalLightRenderPass);
		}

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

		void deinitMainCamPass() {
			RenderSystem* renderSys = RenderSystem::instance();
			renderSys->getRenderPassManager()->unregisterRenderPass(mMainCamPass);
			delete mMainCamPass;
			mMainCamPass = 0;
		}

		void deinit() {
			CameraManager::instance()->UnregisterCamera(mMainCamera);
			delete mMainCamera;
			mMainCamera = nullptr;
			deinitMainCamPass();
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
			auto curScene = Scene::getCurrentScene();
			auto cmdbufOffscreen = RenderSys->GetCommandBufferCurFrameCurThread();

			//1. update camera data.
			CameraManager::instance()->updateAllCamera(cmdbufOffscreen);
			if (curScene) {
				curScene->getLightMgr().update();
			}
			RenderSys->setCurrentCamera(mMainCamera);

			curScene->collectVisibleObjects(mMainCamera);

			RenderSys->cmdBegin(cmdbufOffscreen);
			RenderSys->excutePendingBufferCopies(cmdbufOffscreen);
			/////////////////////////////////////////////////////
			Scene::getCurrentScene()->getLightMgr().calculateIBLData(cmdbufOffscreen);
			//Transit common data
			RenderSys->transitDrawdataResourceState(cmdbufOffscreen, PipelineType::Graphics, Scene::getCurrentScene()->getSceneDrawData());

			DebugDrawManager::instance()->onRender(mMainCamera);
			mMainCamPass->draw(cmdbufOffscreen,mMainCamera);
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
			
			//Wait for last time frame Render finished
			//Like frame 1 is wait for frame 0 to render finished
			if(RenderSys->getNextRenderFrame() != 0)
				RenderSys->waitForFence(mWaitForRenderEndFence,RenderSys->getCurRenderFif());
			RenderSys->resetFence(mWaitForRenderEndFence, RenderSys->getCurRenderFif());
		}
	private:
		MainCameraPass* mMainCamPass = 0;
		PostEffectComposePass* mPostEffectPass = 0;
		RenderPass* mDirectionalLightRenderPass = nullptr;
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
		Camera* mMainCamera = nullptr;

		TexturePtr mMainColorTex = nullptr;
		TexturePtr mMainDepthTex = nullptr;
	};
}

#endif