
#include "Renderer/MaterialTemplate.h"
#include "Renderer/MaterialVarient.h"
#include "Renderer/RenderSystem.h"
#include "vulkan/vulkan_render_function.h"
#include "vulkan/vulkan_pipeline.h"
#include "vulkan/vulkan_shader_module.h"
#include "Renderer/RenderPass.h"
#include <cassert>
namespace Render {

	void MaterialTemplate::onRenderPassRTChangedNeedRebuild(RenderPass* pass)
	{
		if (!pass)return;

		auto ctx = RenderSystem::instance()->getRenderContext();
		//Rebuild all variant with target pass.
		for (auto&& [name, matVariant] : this->mVarientMap) {
			if (matVariant->getRenderPass() == pass) {
				//1. destroy old pipeline.
				auto pipeline = (Vulkan::rs_pipeline_vk*)matVariant->getRsPipeline();
				Vulkan::destroyRsPipeline(ctx, pipeline);
				matVariant->mRsPipeline = createVariantPipeline(pass, matVariant->getShaderStageInfo());
			}
		}


	}

	Material* MaterialTemplate::createVariant(RenderPass* pass, const std::vector<std::pair<ShaderStage, std::string>>& shaderMarco)
	{
		auto ctx = RenderSystem::instance()->getRenderContext();

		auto itor = mVarientMap.find(pass->getPassName());
		if (itor != mVarientMap.end()) {
			destroyVarient(itor->second);
		}
		auto pipeline = createVariantPipeline(pass,shaderMarco);
		Material* mat = new Material(pass,this,pipeline,shaderMarco);

		mVarientMap[pass->getPassName()] = mat;

		return mat;
	}
	Material* MaterialTemplate::getVarient(const Name& name)
	{
		auto itor = mVarientMap.find(name);
		if (itor != mVarientMap.end()) {
			return itor->second;
		}

		return nullptr;
	}
	MaterialTemplate::~MaterialTemplate()
	{
		for (auto& [name, varient] : mVarientMap) {
			destroyVarient(varient);
		}
		mVarientMap.clear();
	}

	rs_pipeline* MaterialTemplate::createVariantPipeline(RenderPass* pass,const std::vector<std::pair<ShaderStage, std::string>>& shaderMarco)
	{

		auto ctx = RenderSystem::instance()->getRenderContext();
		std::vector<rs_shader_module*> modules;
		ShaderCompileDesc compileDesc{};
		ShaderDesc sd{};

		for (auto& [stage, shader] : getShaderStageInfo()) {
			auto File = fopen(shader.c_str(), "r");
			if (!File) {
				assert(0);
				return nullptr;
			}
			fseek(File, 0, SEEK_END);
			uint32_t size = ftell(File);
			rewind(File);
			std::string shaderCode;
			shaderCode.resize(size);
			fread(shaderCode.data(), 1, size, File);
			sd.shaderCode = 0;
			sd.codeSizeByte = 0;
			sd.stage = stage;
			compileDesc.generateDebugInfo = true;
			compileDesc.langType = ShaderLang::GLSL;
			std::string marco;
			for (auto& [mstage, mmarco] : shaderMarco) {
				if (mstage == stage) {
					marco = mmarco;
				}
			}
			compileDesc.shaderSrcCode = marco + shaderCode;
			compileDesc.stage = stage;
			sd.compileDesc = &compileDesc;
			modules.push_back(Vulkan::createRsShader(ctx, sd));
		}

		PipelineDesc desc{
			.type = PipelineType::Graphics,
			.shaders = modules,
			.vertexInputDesc = getInputVertexDesc()
		};

		auto pipeline = Vulkan::createRsPipeline(ctx, (Vulkan::rs_renderpass_vk*)pass->getRaw(), desc);
		for (auto sdm : modules) {
			auto mod = (Vulkan::rs_shader_module_vk*)sdm;
			Vulkan::destroyRsShader(ctx, mod);
		}
		return pipeline;

	}

	void MaterialTemplate::destroyVarient(Material* mat)
	{
		auto ctx = RenderSystem::instance()->getRenderContext();
		auto pipeline = (Vulkan::rs_pipeline_vk*)mat->getRsPipeline();
		Vulkan::destroyRsPipeline(ctx, pipeline);
		delete mat;
	}

}
