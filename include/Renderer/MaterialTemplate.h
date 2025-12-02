#ifndef MATERIAL_TEMPLATE_H
#define MATERIAL_TEMPLATE_H
#include <vector>
#include <map>
#include "common/Name.h"
#include "render_resource_createinfo.h"
namespace Render {
	using ShaderStageInfo = std::vector<std::pair<ShaderStage, std::string>>;
	class Material;
	class RenderPass;
	class MaterialTemplate {
	public:
		MaterialTemplate(const ShaderStageInfo& shaderInfo, const RenderState& state, const VertexInputDescription& inputDesc):mRenderState(state),mShaderInfo(shaderInfo), mInputDesc(inputDesc){
		}
		const RenderState& getRenderState()const { return mRenderState; }
		const ShaderStageInfo& getShaderStageInfo()const { return mShaderInfo; }
		const VertexInputDescription& getInputVertexDesc()const {
			return mInputDesc;
		}

		Material* createVarient(RenderPass* pass, const std::vector<std::pair<ShaderStage, std::string>>& shaderMarco);
		Material* getVarient(const Name& passName);
	
		~MaterialTemplate();
	private:
		void destroyVarient(Material* material);
		std::map<const Name&, Material*> mVarientMap;

		RenderState mRenderState;
		ShaderStageInfo mShaderInfo;
		VertexInputDescription mInputDesc;
	};
}

#endif