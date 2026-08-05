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
#include "Renderer/RenderPass/DebugOverlayPass.h"
#include "Renderer/PostEffect/CODBloom.h"
#include "Renderer/TextureResourceMgr.h"
#include "Renderer/Blit.h"
#include "Renderer/EnginePass.h"
#include "Renderer/DebugDrawManager.h"
#include "Renderer/RenderPass/ShadowPass.h"
#include "Renderer/ConstShaderDataManager.h"
#include "Renderer/UI/ImGuiManager.h"
#include "Renderer/HiZ.h"
#include "Renderer/ClusteredLights.h"
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
			initPreZPass();
			initPostEffectPass();
			initDebugOverlayPass();
			this->mOffscreenFinishSemaphore = RenderSystem::instance()->createSemaphore();
			mWaitForRenderEndFence = RenderSystem::instance()->createFence();
			auto RenderSys = RenderSystem::instance();
			mMainCamera = new Camera(Name("SceneMainCamera"));
			mMainCamera->setCullMask(CullMask::MainCamera);
			CameraManager::instance()->RegisterCamera(mMainCamera, 0);
			DebugDrawManager::instance()->init();
			mBloom = new CodBloom();
			mBloom->setBloomRadius(1.5);
			mHizClusterLights = new HizClusteredLight();
			mHiZ = new HiZ;

;		}

		void deinitDirectionalLightShadowPass() {
			auto rsys = RenderSystem::instance();
			rsys->getRenderPassManager()->unregisterRenderPass(mDirectionalLightRenderPass);
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
			auto postEffectImage = rsys->createImage2D(0,0,ImageFormat::RGBA8_UNORM, MAIN_RT_SIZE_X, MAIN_RT_SIZE_Y, 1, 1, 1, ImageUsage_TransferSrc | ImageUsage_ColorAttachment | ImageUsage_Storage);
			postEffectTexture = TextureResourceManager::instance()->createEmpty();
			postEffectTexture->setRsImage(postEffectImage);
			postEffectRenderTarget = rsys->createRendertarget({ postEffectImage }, nullptr);
			mPostEffectPass->setRenderTarget(postEffectRenderTarget);
			mPostEffectPass->init();
		}

		void deinitPostEffectPass() {
			auto rsys = RenderSystem::instance();
			postEffectTexture = nullptr;
			rsys->destroyRenderTarget(postEffectRenderTarget);

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
			auto mainDepthImg = rsys->createDepthStencilTexture(RenderTextureFormat::D24S8, MAIN_RT_SIZE_X, MAIN_RT_SIZE_Y, true);
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

		void initDebugOverlayPass() {
			RenderSystem* renderSys = RenderSystem::instance();
			mOverlayRenderPass = new DebugOverlayPass;
			renderSys->getRenderPassManager()->registerRenderPass(mOverlayRenderPass);
			mOverlayRenderPass->init();
		}

		void deinitDebugOverlayPass() {
			RenderSystem* renderSys = RenderSystem::instance();
			renderSys->getRenderPassManager()->unregisterRenderPass(mOverlayRenderPass);
			mOverlayRenderPass->deinit();
			delete mOverlayRenderPass;
		}

		void deinit() {
			delete mHiZ;
			CameraManager::instance()->UnregisterCamera(mMainCamera);
			delete mMainCamera;
			mMainCamera = nullptr;
			deinitPreZ();
			deinitDirectionalLightShadowPass();
			deinitMainCamPass();
			deinitPostEffectPass();
			deinitDebugOverlayPass();
			RenderSystem::instance()->destroySemaphore(mOffscreenFinishSemaphore);
			RenderSystem::instance()->destroyFence(mWaitForRenderEndFence);
		}

		void initPreZPass() {

			//***************************************************************//
			
			//			CHECK OUT MAIN CAM PASS TO ENABLE PREZ				 // 
			
			//***************************************************************//
			//Render Motion vector during prez pass
			LogicPassDesc logicPass{};
			logicPass.drawOrder = PassDrawOrder::FromFarToNear;
			logicPass.filterMask = RenderMask::Normal;
			logicPass.logicPassName = PassName::PreZPass;
			StageMacroPairs pairs{};
			pairs.push_back(
				{ ShaderStage::Fragment,{{"PREZ","1"}} }
			);
			logicPass.passMacros = pairs;
			PassDesc prezDesc{};
			prezDesc.lastDepth = true;
			PassAttachment attachmentMotionVec{};

			attachmentMotionVec.fmt = RenderTextureFormat::RG16F;
			attachmentMotionVec.loadOp = StorageOp::Clear;
			attachmentMotionVec.storeOp = StorageOp::Cached;
			prezDesc.attachments.push_back(attachmentMotionVec);

			PassAttachment attachmentDepth{};
			attachmentDepth.fmt = RenderTextureFormat::D24S8;
			attachmentDepth.loadOp = StorageOp::Clear;
			attachmentDepth.storeOp = StorageOp::Cached;

			prezDesc.attachments.push_back(attachmentDepth);

			mPrezRenderPass = new RenderPass(
				{ logicPass }, prezDesc
			);
			RenderSystem::instance()->getRenderPassManager()->registerRenderPass(mPrezRenderPass);
			mMotionVector = TextureResourceManager::instance()->createRenderTexture(RenderTextureFormat::RG16F, MAIN_RT_SIZE_X, MAIN_RT_SIZE_Y,1,1,1,true);
			mPrePassRt = RenderSystem::instance()->createRendertarget({ mMotionVector->getRsImage() }, mMainDepthTex->getRsImage());
			mPrezRenderPass->setRenderTarget(mPrePassRt);
			ClearColor motionVecClear{};
			ClearDepthStencil depthClean{};
			depthClean.depth	= 1.;
			depthClean.stencil	= 0.;
			mPrezRenderPass->setClearData({ motionVecClear }, depthClean);
		}

		void deinitPreZ() {
			RenderSystem::instance()->getRenderPassManager()->unregisterRenderPass(mPrezRenderPass);
			RenderSystem::instance()->destroyRenderTarget(mPrePassRt);
			mPrePassRt = nullptr;
			mPrezRenderPass = nullptr;
			delete mPrezRenderPass;
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

			if (curScene) {
				//Bind Lights
				ConstShaderDataManager::instance()->updateLightsData(curScene);
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
			
			mPrezRenderPass->draw(cmdbufOffscreen, mMainCamera);
			mHiZ->setDepth(mMainDepthTex);
			mHiZ->execute(cmdbufOffscreen);

			if (curScene)
			{
				mHizClusterLights->setHiZTexture(mHiZ->getHiZPyramid());
				mHizClusterLights->setLightListBuffer(ConstShaderDataManager::instance()->getLightDataBuffer(), curScene->getLightMgr().getLightData().size());
				mHizClusterLights->draw(cmdbufOffscreen, mMainCamera, curScene);
				ConstShaderDataManager::instance()->setClusterLightsData(mHizClusterLights, curScene);
			}

			DebugDrawManager::instance()->onRender(mMainCamera);
			mMainCamPass->draw(cmdbufOffscreen, mMainCamera);

			mBloom->draw(cmdbufOffscreen, mMainColorTex);

			mPostEffectPass->setBloomTex(mBloom->outBloomTex());
			mPostEffectPass->setMainRTColorTex(mMainColorTex);
			mPostEffectPass->draw(cmdbufOffscreen);

			mOverlayRenderPass->setGameView(postEffectTexture);
			mOverlayRenderPass->draw(cmdbufOffscreen, nullptr);

			RenderSys->cmdEnd(cmdbufOffscreen);

			RenderSys->submitCmdBuffer(cmdbufOffscreen, {}, { RenderSys->getRenderFinishSemaphore() }, nullptr);
			RenderSys->blitToSwapchain(mOverlayRenderPass->getOverlayTexture()->getRsImage());
			//RenderSys->waitLastRenderEnd();
			//Then into real render frame!
		}
	private:
		DebugOverlayPass* mOverlayRenderPass = 0;
		MainCameraPass* mMainCamPass = 0;
		PostEffectComposePass* mPostEffectPass = 0;
		RenderPass* mDirectionalLightRenderPass = nullptr;
		RenderPass* mPrezRenderPass = nullptr;
		std::vector<RenderEntity*> mRenderEntities;
		std::vector<RenderEntity*> mPostEffectEntities;
		
		rs_semaphore* mOffscreenFinishSemaphore;
		rs_fence* mWaitForRenderEndFence;
		RenderEntity* BlitEntity;
		MaterialTemplate* BlitMaterial;

		rs_image* mainDepthImg = nullptr;
		rs_rendertarget* mRenderTarget = nullptr;
		
		TexturePtr postEffectTexture = nullptr;
		rs_rendertarget* postEffectRenderTarget = nullptr;

		CodBloom* mBloom = nullptr;
		HiZ* mHiZ = nullptr;
		HizClusteredLight* mHizClusterLights = nullptr;
		Camera* mMainCamera = nullptr;

		TexturePtr mMainColorTex = nullptr;
		TexturePtr mMainDepthTex = nullptr;
		TexturePtr mMotionVector = nullptr;
		rs_rendertarget* mPrePassRt = nullptr;
	};
}

#endif