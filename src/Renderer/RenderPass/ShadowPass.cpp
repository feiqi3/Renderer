#include "Renderer/RenderPass/ShadowPass.h"
#include "Renderer/EnginePass.h"

namespace Render {
	
	static PassDesc getShadowPassDesc() {
		PassDesc desc{};
		PassAttachment att{};
		att.fmt = RenderTextureFormat::D32;
		att.isHDR = false;
		att.loadOp = StorageOp::Clear;
		att.storeOp = StorageOp::Cached;
		desc.attachments.push_back(att);
		desc.lastDepth = true;
		return desc;
	};
	
	DirLightShadowPass::DirLightShadowPass() : RenderPass(PassName::DirectionalShadowPass,getShadowPassDesc())
	{
		
	}

	Render::StageMacroPairs DirLightShadowPass::getPassStageShaderMacro(const Name& logicPassName)
	{
		return {
			{
				ShaderStage::Vertex, {
					{"SHADOW_PASS",""},
					{"DIR_LIGHT_SHADOW",""},
					{"NO_SHADOW",""}
				}
			},			
			{
				ShaderStage::Fragment, {
					{"SHADOW_PASS",""},
					{"DIR_LIGHT_SHADOW",""},
					{ "NO_SHADOW","" }
				}
			},
		};
	}

}
