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
#include "Renderer/RenderPass/ShadowPass.h"
#include "Renderer/ConstShaderDataManager.h"

#define MAIN_RT_SIZE_X 1024
#define MAIN_RT_SIZE_Y 1024

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
			initDirectionalLightShadowPass();
			initMainCamPass();
			initPostEffectPass();

			this->mOffscreenFinishSemaphore = RenderSystem::instance()->createSemaphore();
			mWaitForRenderEndFence = RenderSystem::instance()->createFence();
			auto RenderSys = RenderSystem::instance();
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

			mDirectionalLightRenderPass = new DirLightShadowPass();
			ClearDepthStencil clearData{};
			clearData.depth = 1.;
			mDirectionalLightRenderPass->setClearData({}, clearData);
			rsys->getRenderPassManager()->registerRenderPass(mDirectionalLightRenderPass);
		}

		void initPostEffectPass() {
			auto rsys = RenderSystem::instance();
			rsys->getRenderPassManager()->registerRenderPass(mPostEffectPass);

			//create post effect rt 
			postEffectImage = rsys->createImage2D(0,0,ImageFormat::RGBA8_UNORM, MAIN_RT_SIZE_X, MAIN_RT_SIZE_Y, 1, 1, 1, ImageUsage_TransferSrc | ImageUsage_ColorAttachment);
			postEffectRenderTarget = rsys->createRendertarget({ postEffectImage }, nullptr);
			mPostEffectPass->setRenderTarget(postEffectRenderTarget);
			mPostEffectPass->init();
		}

		void deinitPostEffectPass() {
			auto rsys = RenderSystem::instance();
			rsys->destroyRenderTarget(postEffectRenderTarget);
			rsys->destroyImage(postEffectImage);

			rsys->getRenderPassManager()->unregisterRenderPass(mPostEffectPass);
			delete mPostEffectPass;
			mPostEffectPass = nullptr;
		}

		void initMainCamPass() {
			mMainCamPass->init();
			auto rsys = RenderSystem::instance();
			rsys->getRenderPassManager()->registerRenderPass(mMainCamPass);
			auto mainColorImg = rsys->createRTTexture(RenderTextureFormat::RGBA16F, MAIN_RT_SIZE_X, MAIN_RT_SIZE_Y, 1, 1 ,1 , true);
			mMainColorTex = TextureResourceManager::instance()->createFromRsImage(Name("MainColorTexture"),mainColorImg);
			auto mainDepthImg = rsys->createDepthStencilTexture(RenderTextureFormat::D24S8, MAIN_RT_SIZE_X, MAIN_RT_SIZE_Y, false);
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
			initDirectionalLightShadowPass();
			deinitMainCamPass();
			deinitPostEffectPass();
			RenderSystem::instance()->destroySemaphore(mOffscreenFinishSemaphore);
			RenderSystem::instance()->destroyFence(mWaitForRenderEndFence);
		}

		void AddEntity(RenderEntity* entity) {
			mRenderEntities.push_back(entity);
		}

		void Excute(){
			
			auto RenderSys = RenderSystem::instance();
			auto curScene = Scene::getCurrentScene();

			auto cmdbufOffscreen = RenderSys->GetCommandBufferCurFrameCurThread();

			if (curScene) {
				curScene->getLightMgr().update();
			}
			CameraManager::instance()->updateAllCamera(cmdbufOffscreen);
			RenderSys->setCurrentCamera(mMainCamera);
			curScene->collectVisibleObjects(mMainCamera);

			RenderSys->cmdBegin(cmdbufOffscreen);
			RenderSys->excutePendingBufferCopies(cmdbufOffscreen);

			if (curScene) {
				curScene->getLightMgr().calculateIBLData(cmdbufOffscreen);
				curScene->getShadowMgr().drawShadow(cmdbufOffscreen, mMainCamera, curScene);
				ConstShaderDataManager::instance()->updateShadowDrawData(curScene);
			}

			DebugDrawManager::instance()->onRender(mMainCamera);
			mMainCamPass->draw(cmdbufOffscreen, mMainCamera);

			mBloom->draw(cmdbufOffscreen, mMainColorTex);

			mPostEffectPass->setBloomTex(mBloom->outBloomTex());
			mPostEffectPass->setMainRTColorTex(mMainColorTex);
			mPostEffectPass->draw(cmdbufOffscreen);

			RenderSys->cmdEnd(cmdbufOffscreen);

			RenderSys->submitCmdBuffer(cmdbufOffscreen, {}, { RenderSys->getRenderFinishSemaphore() }, nullptr);
			RenderSys->blitToSwapchain(postEffectImage);
			RenderSys->waitLastRenderEnd();
			//Then into real render frame!
		}
	private:
		MainCameraPass* mMainCamPass = 0;
		PostEffectComposePass* mPostEffectPass = 0;
		RenderPass* mDirectionalLightRenderPass = nullptr;
		std::vector<RenderEntity*> mRenderEntities;
		std::vector<RenderEntity*> mPostEffectEntities;
		
		rs_semaphore* mOffscreenFinishSemaphore;
		rs_fence* mWaitForRenderEndFence;
		RenderEntity* BlitEntity;
		MaterialTemplate* BlitMaterial;

		rs_image* mainDepthImg = nullptr;
		rs_rendertarget* mRenderTarget = nullptr;
		
		rs_image* postEffectImage = nullptr;
		rs_rendertarget* postEffectRenderTarget = nullptr;

		CodBloom* mBloom = nullptr;
		Camera* mMainCamera = nullptr;

		TexturePtr mMainColorTex = nullptr;
		TexturePtr mMainDepthTex = nullptr;
	};
}

#endif