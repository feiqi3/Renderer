#include "Renderer/MaterialVarient.h"
#include "vulkan/vulkan_render_function.h"
#include "vulkan/vulkan_pipeline.h"
#include "vulkan/vulkan_shader_module.h"
#include "Renderer/RenderPass.h"
#include "Renderer/RenderSystem.h"
#include <stdio.h>
namespace Render {

	Material::Material(RenderPass* renderPass, MaterialTemplate* fromTemplate, rs_pipeline* pipeline, const ShaderStageInfo& stageInfo):mRenderPass(renderPass),mMaterialTemplate(fromTemplate), mRsPipeline(pipeline),mShaderMacro(stageInfo)
	{
		mRsPipeline = pipeline;
		//Construct a binding table
		for (auto&& binding : pipeline->bindingInfo) {
			mBindingTable.insert({ binding.bindingItemName,binding });
		}
	}

}
