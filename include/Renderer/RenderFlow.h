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
		}

		void initPostEffectPass();

		void initMainCamPass() {
			mMainCamRenderTexture = RenderSystem::instance()->createRTTexture(Render::ImageFormat::BGRA8_UNORM, 1920, 1080, 1, 1, false);
			mRenderTarget = RenderSystem::instance()->createRendertarget({ mMainCamRenderTexture }, 0);
			mMainCamPass->setRenderTarget(mRenderTarget);
			mMainCamPass->init();
		}

		void initSwapChainPass() {
			RenderSystem* renderSys = RenderSystem::instance();
			auto SwapImg = renderSys->getSwapchainImage(renderSys->getCurFif());
			mSwapchainPass->setRenderTarget(renderSys->getNextSwapchainRendertarget());
			mSwapchainPass->init();
		};

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
		}

		void AddEntity(RenderEntity* entity) {
			mRenderEntities.push_back(entity);
		}

		void Excute(){
			auto RenderSys = RenderSystem::instance();
			auto cmdbuf = RenderSys->GetCommandBufferCurFrameCurThread();
			RenderSys->cmdBegin(cmdbuf);
			for (auto&& entity : mRenderEntities) {
				mMainCamPass->addToDraw(entity);
			}
			mMainCamPass->draw(cmdbuf);
			mSwapchainPass->setRenderTarget(RenderSys->getNextSwapchainRendertarget());
			mSwapchainPass->draw(cmdbuf);
			RenderSys->cmdEnd(cmdbuf);
		}
	private:
		MainCamPass* mMainCamPass = 0;
		rs_image* mMainCamRenderTexture = 0;
		rs_rendertarget* mRenderTarget = 0;
		SwapchainPass* mSwapchainPass = 0;
		std::vector<RenderEntity*> mRenderEntities;
		std::vector<RenderEntity*> mPostEffectEntities;
		
		RenderEntity* BlitEntity;
		MaterialTemplate* BlitMaterial;
	};
}

#endif