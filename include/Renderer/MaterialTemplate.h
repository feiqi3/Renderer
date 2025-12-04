#ifndef MATERIAL_TEMPLATE_H
#define MATERIAL_TEMPLATE_H
#include <string>
#include <vector>
#include <map>
#include "common/Name.h"
#include "render_resource_createinfo.h"
namespace Render {
	class Material;
	class RenderPass;
	struct rs_pipeline;
	class MaterialTemplate {
	public:
		MaterialTemplate(const Name& name, const ShaderStageInfo& shaderInfo, const RenderState& state, const VertexInputDescription& inputDesc):mName(name),mRenderState(state),mShaderInfo(shaderInfo), mInputDesc(inputDesc){
		}
		const RenderState& getRenderState()const { return mRenderState; }
		const ShaderStageInfo& getShaderStageInfo()const { return mShaderInfo; }
		const VertexInputDescription& getInputVertexDesc()const {
			return mInputDesc;
		}
		virtual void onRenderPassRTChangedNeedRebuild(RenderPass* pass);
		Material* createVariant(RenderPass* pass, const std::vector<std::pair<ShaderStage, std::string>>& shaderMarco);
		Material* getVarient(const Name& passName);
		const Name& getName()const { return mName; }
	private:
		rs_pipeline* createVariantPipeline(RenderPass* pass, const std::vector<std::pair<ShaderStage, std::string>>& shaderMarco);
		void destroyVarient(Material* material);
		~MaterialTemplate();
	private:
		friend class MaterialTemplateManager;
		Name mName;
		std::map<Name, Material*> mVarientMap;
		RenderState mRenderState;
		ShaderStageInfo mShaderInfo;
		VertexInputDescription mInputDesc;
	};
}

#endif