#include "Renderer/MaterialTemplateManager.h"
#include "Renderer/MaterialTemplate.h"
#include "Renderer/RenderPass.h"
#include <exception>
#include <stdexcept>

namespace Render {

	MaterialTemplate* MaterialTemplateManager::createMaterialTemplate(const Name& templateName, const ShaderStageInfo& shaderInfo, const RenderState& state, const VertexInputDescription& inputDesc)
	{
		auto itor = mMaterialTemplates.find(templateName);
		if (itor != mMaterialTemplates.end()) {
			assert(0 && "Error: duplicated material");
			throw std::runtime_error("Error: duplicated material template name");
			return nullptr;
		}
		auto mt = new MaterialTemplate(templateName, shaderInfo, state, inputDesc);
		this->mMaterialTemplates.insert({ templateName, mt });
		return mt;
	}

	MaterialTemplateManager::~MaterialTemplateManager()
	{
		for (auto& [name, mat] : this->mMaterialTemplates) {
			delete mat;
			mat = 0;
		}
		mMaterialTemplates.clear();
	}

	void MaterialTemplateManager::destroyMaterialTemplate(const Name& templateName)
	{
		auto itor = mMaterialTemplates.find(templateName);
		if (itor != mMaterialTemplates.end()) {
			delete itor->second;
		}
		mMaterialTemplates.erase(itor);
	}

	void MaterialTemplateManager::broadcastPipelineRebuild(RenderPass* passChanged)
	{
		for (auto& [name, mat] : this->mMaterialTemplates) {
			mat->onRenderPassRTChangedNeedRebuild(passChanged);
		}
	}

}
