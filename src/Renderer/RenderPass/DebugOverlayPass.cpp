#include "Renderer/RenderPass/DebugOverlayPass.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/EnginePass.h"
#include "Renderer/UI/ImGuiManager.h"
#include "Renderer/RenderDebuger.h"
namespace Render {

	static PassDesc getPassDesc() {
		PassDesc desc;
		PassAttachment att{};
		att.fmt = RenderTextureFormat::RGBA8;
		att.storeOp = StorageOp::Cached;
		att.loadOp = StorageOp::Cached;
		desc.attachments.push_back(att);
		desc.lastDepth = false;
		desc.writeDepth = false;
		return desc;
	}

	DebugOverlayPass::DebugOverlayPass() : RenderPass(PassName::GUIOverlay, getPassDesc())
	{
		ClearColor clear{};
		clear.rgba[0] = 0.;
		clear.rgba[1] = 0.;
		clear.rgba[2] = 0.;
		//We need alpha to be zero, which will be convience for the following compose pass.
		clear.rgba[3] = 0.;
		this->setClearData({ clear }, {});
	}

	DebugOverlayPass::~DebugOverlayPass()
	{
		RenderSystem::instance()->destroyRenderTarget(mRenderTarget);
		mRenderTarget = nullptr;
	}

	void DebugOverlayPass::init()
	{
		IMGUIManager::instance()->init();
	}

	void DebugOverlayPass::deinit()
	{
		IMGUIManager::instance()->deinit();
	}

	void DebugOverlayPass::preDraw(rs_commandbuffer* cmdbuffer, Camera* cam)
	{
		if (!mRenderTexture) {
			mRenderTexture = TextureResourceManager::instance()->createEmpty();
		}
		//1. get win size 
		int winx, winy;
		RenderSystem::instance()->getWindowSize(winx, winy);
		if (winx * winy != 0 && (winx != lastFrameWinX || winy != lastFrameWinY)) {
			lastFrameWinX = winx;
			lastFrameWinY = winy;
			auto img = RenderSystem::instance()->createImage2D(
				0, 0, ImageFormat::RGBA8_UNORM, lastFrameWinX, lastFrameWinY, 1, 1, 1, ImageUsage_Sampled | ImageUsage_ColorAttachment | ImageUsage_TransferSrc | ImageUsage_Storage
			);
			if (mRenderTarget) {
				RenderSystem::instance()->destroyRenderTarget(mRenderTarget);
			}
			mRenderTarget = RenderSystem::instance()->createRendertarget(
				{ img }, nullptr
			);
			mRenderTexture->setRsImage(img, true);
		}
	}

	void DebugOverlayPass::draw(rs_commandbuffer* cmdbuffer, Camera* cam)
	{
		RenderMarker marker(cmdbuffer, "OverlayPass", 0.6, 0.3, 0.3, 1.);
		preDraw(cmdbuffer,cam);
		auto RenderSys = RenderSystem::instance();
		RenderSys->setCurrentCamera(nullptr);

		//Blit gameview to cur ui render target
		if (mGameView) {
			RenderSys->cmdBlitCompute(cmdbuffer, mGameView, mGameView->getRsImage()->defaultView->viewKey,
				mRenderTexture, mRenderTexture->getRsImage()->defaultView->viewKey, Filter::Linear
			);

		}


		for (auto& entity : mEntities) {
			RenderSys->updateParameters(cmdbuffer, entity, entity->getPass(this->getPassName()));
		}


		RenderSys->excutePendingBufferCopies(cmdbuffer);

		RenderSys->cmdBeginRenderPass(cmdbuffer, mRenderPass, mClrColor, mDsClear);
		RenderSys->cmdSetRendertarget(cmdbuffer, mRenderTarget);
		Rect2D nextRenderArea{};
		updateViewportAndScissor(cmdbuffer, mRenderTarget);


		//Draw ui over game view

		for (auto entity : mEntities) {
			RenderSys->drawIndexed(cmdbuffer, entity, cam, entity->getPass(this->getPassName()));
		}
		RenderSys->cmdEndRenderPass(cmdbuffer);
		mEntities.clear();
	}

	void DebugOverlayPass::setGameView(TexturePtr image)
	{
		mGameView = image;
	}

	void DebugOverlayPass::addDrawEntity(RenderEntity* entity)
	{
		mEntities.push_back(entity);
	}

	Render::TexturePtr DebugOverlayPass::getOverlayTexture()
	{
		return mRenderTexture;
	}

}


