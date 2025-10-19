#include "Renderer/MainCameraRenderPass.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/RenderEntity.h"
namespace Render {

	static PassDesc MainCamPassDesc{
		.attachments = {
			PassAttachment{
				.loadOp = StorageOp::DontCare,
				.storeOp = StorageOp::Cached
			}
		}
	};

	MainCamPass::MainCamPass():RenderPass("MainPass", MainCamPassDesc)
	{
	}
	void MainCamPass::drawImpl(rs_commandbuffer* cmdbuffer)
	{
		auto renderSys = RenderSystem::instance();
		for (auto&& render : mSceneEntity) {
			renderSys->drawIndexed(cmdbuffer, render, "MainCam");
		}
	}
	void MainCamPass::addToDraw(RenderEntity* Entity)
	{
		this->mSceneEntity.push_back(Entity);
	}
}
