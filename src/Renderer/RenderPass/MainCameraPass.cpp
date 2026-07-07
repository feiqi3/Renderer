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

	static std::vector<LogicPassDesc> getMainCamLogicPassDesc() {
		LogicPassDesc logicPassDesc{};
		std::vector<LogicPassDesc> descs{};
		
		//1. Main cam opaque pass 
		logicPassDesc.filterMask = RenderMask::Normal;
		logicPassDesc.priority = 5;
		logicPassDesc.logicPassName = PassName::MainCameraPass;
		descs.push_back(logicPassDesc);
		//2. Skybox pass
		logicPassDesc.filterMask = RenderMask::SkyBox;
		logicPassDesc.priority = 10;
		logicPassDesc.logicPassName = PassName::SkyboxPass;
		descs.push_back(logicPassDesc);
		//3. Main cam transparent pass
		logicPassDesc.filterMask = RenderMask::Transparent | RenderMask::DebugDraw;
		logicPassDesc.priority = 15;
		logicPassDesc.logicPassName = PassName::MainCameraTransparentPass;
		logicPassDesc.drawOrder = PassDrawOrder::FromFarToNear;
		descs.push_back(logicPassDesc);
		return descs;

	};

	MainCameraPass::MainCameraPass():RenderPass(getMainCamLogicPassDesc(), getMainCamPassDesc())
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