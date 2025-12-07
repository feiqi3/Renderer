#include "Renderer/RenderPass/MainCameraPass.h"
#include "Renderer/RenderSystem.h"
namespace Render{
	static PassDesc getMainCamPassDesc() {
		PassDesc desc{};
		PassAttachment attachmentMain{ 
			.loadOp =StorageOp::Clear,.storeOp = StorageOp::Cached,.isHDR = false, };
		PassAttachment attachmentDepth{
		.loadOp = StorageOp::Clear,.storeOp = StorageOp::Cached,.isHDR = false, };
		desc.attachments = { attachmentMain,attachmentDepth };
		desc.writeDepth = true;
		return desc;
	}

	MainCameraPass::MainCameraPass():RenderPass(Name("MainCameraPass"),getMainCamPassDesc())
	{
	}

	void MainCameraPass::drawImpl(rs_commandbuffer* cmdbuffer)
	{
		auto renderSys = RenderSystem::instance();
		for (auto&& render : mSceneEntity) {
			renderSys->drawIndexed(cmdbuffer, render, this->getPassName());
		}
		mSceneEntity.clear();
	}
	void MainCameraPass::addToDrawList(RenderEntity* Entity)
	{
		this->mSceneEntity.push_back(Entity);
	}
}