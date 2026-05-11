#include "Renderer/RenderPass/PostEffectComposePass.h"
#include "Renderer/RenderEntity.h"
#include "Renderer/EnginePass.h"
#include "Renderer/MaterialInstance.h"
#include "Renderer/MaterialTemplateManager.h"

namespace Render {

	static inline PassDesc getPassDesc() {
		PassDesc desc{};
		PassAttachment attCol{};
		attCol.fmt = RenderTextureFormat::SwapchainFormat;
		attCol.isHDR = false;
		attCol.loadOp = StorageOp::Clear;
		attCol.storeOp = StorageOp::Cached;
		desc.attachments.push_back(attCol);
		desc.lastDepth = false;
		desc.writeDepth = false;
		return desc;
	}
	
	MaterialPtr createPostEffectComposeMaterial(RenderPass* rp) {
		RenderState postEffectRenderState{};
		postEffectRenderState.cullMode = CullMode::None;
		postEffectRenderState.depthTestEnable  = false;
		postEffectRenderState.depthWriteEnable = false;

		
		auto matTemp = MaterialTemplateManager::instance()->createMaterialTemplate(
			Name("PostEffectCompose"), {
				{ShaderStage::Vertex,"../shader/PostEffect/FullScreen.vs"},
				{ShaderStage::Fragment, "../shader/PostEffect/PostEffectCompose.ps"}
			}, postEffectRenderState, {}
		);

		matTemp->createMaterialPass(rp);
		MaterialManager::instance()->createMaterial<Material>(Name("PostEffeectCompose"), matTemp);
	}

	class PostEffectComposeEntity : public RenderEntity {
	public:
		Material* getMaterial()override { return mMaterial.get(); }

	private:
		MaterialPtr mMaterial;
	};

	PostEffectComposePass::PostEffectComposePass() : RenderPass(PassName::PostEffectComposePass,getPassDesc())
	{
		
	}

	PostEffectComposePass::~PostEffectComposePass()
	{

	}

}