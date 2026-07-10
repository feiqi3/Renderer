#include "Renderer/MainCameraRenderPass.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/RenderEntity.h"
namespace Render {

	static PassDesc MainCamPassDesc{
		.attachments = {
			PassAttachment{
				.fmt	= RenderTextureFormat::RGBA8,
				.loadOp = StorageOp::Clear,
				.storeOp = StorageOp::Cached
			},
			PassAttachment{
				.fmt = RenderTextureFormat::D24S8,
				.loadOp = StorageOp::Clear,
				.storeOp = StorageOp::DontCare
			},
		}
	};

	MainCamPass::MainCamPass():RenderPass(Name("MainPass"), MainCamPassDesc)
	{
	}
	void MainCamPass::drawImpl(rs_commandbuffer* cmdbuffer,Camera* camera)
	{
		auto renderSys = RenderSystem::instance();
		for (auto&& render : mSceneEntity) {
			renderSys->drawIndexed(cmdbuffer, render,camera, this->getPassName());
		}
		mSceneEntity.clear();
	}
	void MainCamPass::addToDraw(RenderEntity* Entity)
	{
		this->mSceneEntity.push_back(Entity);
	}
}
