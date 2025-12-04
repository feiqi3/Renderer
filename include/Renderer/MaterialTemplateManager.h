#ifndef MATERIAL_TEMPLATE_MANAGER_H_
#define MATERIAL_TEMPLATE_MANAGER_H_
#include "common/Singleton.h"
#include "render_resource_createinfo.h"
#include "common/Name.h"
#include <map>
namespace Render {
	class MaterialTemplate;
	class Material;
	class RenderPass;
	class MaterialTemplateManager :public Singleton< MaterialTemplateManager> {
	public:
		MaterialTemplate* createMaterialTemplate(const Name& templateName, const ShaderStageInfo& shaderInfo, const RenderState& state, const VertexInputDescription& inputDesc);
		~MaterialTemplateManager();
		void destroyMaterialTemplate(const Name& templateName);
		void broadcastPipelineRebuild(RenderPass* passChanged);
	private:
		std::map<Name, MaterialTemplate*> mMaterialTemplates;
	};
};

#endif
