#include "Renderer/RenderPass/MainCameraPass.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/RenderQueue.h"
#include "Renderer/EnginePass.h"
namespace Render{
	static PassDesc getMainCamPassDesc() {
		PassDesc desc{};
		PassAttachment attachmentMain{ 
			.fmt = RenderTextureFormat::RGBA16F, .loadOp = StorageOp::Clear, .storeOp = StorageOp::Cached, .isHDR = true, };
		PassAttachment attachmentDepth{
			.fmt = RenderTextureFormat::D24S8, .loadOp = StorageOp::Clear, .storeOp = StorageOp::Cached, .isHDR = false, };
		desc.attachments = { attachmentMain,attachmentDepth };
		desc.lastDepth  = true;
		desc.writeDepth = true;
		return desc;
	}

	MainCameraPass::MainCameraPass():RenderPass(PassName::MainCameraPass,getMainCamPassDesc())
	{
		ClearColor mainColorAtt = {};
		mainColorAtt.rgba[0] = 0.f;
		mainColorAtt.rgba[1] = 0.f;
		mainColorAtt.rgba[2] = 0.f;
		mainColorAtt.rgba[3] = 1.f;
		ClearDepthStencil depthClearCol;
		depthClearCol.depth = 1.f;
		depthClearCol.stencil = 0;
		this->setClearData({ mainColorAtt }, depthClearCol);
	}

}