
#include "Renderer/MaterialTemplate.h"
#include "Renderer/MaterialVarient.h"
#include "Renderer/RenderSystem.h"
#include "vulkan/vulkan_render_function.h"
#include "vulkan/vulkan_pipeline.h"
#include "vulkan/vulkan_shader_module.h"
#include "Renderer/RenderPass.h"
#include <cassert>
namespace Render {

	MaterialTemplate::MaterialTemplate(const ShaderStageInfo& shaderInfo, const RenderState& state, const VertexInputDescription& inputDesc)
		: mRenderState(state), mShaderInfo(shaderInfo), mInputDesc(inputDesc)
	{
		mState = ResourceState::Loaded;
	}

	const Name& MaterialTemplate::typeName()
	{
		static const Name typeName("MaterialTemplate");
		return typeName;
	}
	const Name& MaterialTemplate::getTypeName() const {
		return typeName();
	}

	ResourceMemory MaterialTemplate::getMemory() const {
		//A rought count of memory usage
		ResourceMemory mem{ 0, 0 };
		mem.cpuMemory = (uint32_t)sizeof(*this);
		mem.cpuMemory += this->mMaterialPassMap.size() * sizeof(MaterialPass);

		return mem;
	}

	void MaterialTemplate::OnUnload() {
		for (auto&& [name, mat] : mMaterialPassMap) {
			destroyMaterialPass(mat);
		}
		mMaterialPassMap.clear();
		mState = ResourceState::Unloaded;
	}

	void MaterialTemplate::onRenderPassRTChangedNeedRebuild(RenderPass* pass)
	{
		if (!pass)return;

		auto ctx = RenderSystem::instance()->getRenderContext();
		//Rebuild all variant with target pass.
		for (auto&& [name, matVariant] : this->mMaterialPassMap) {
			if (matVariant->getRenderPass() == pass) {
				//1. destroy old pipeline.
				auto pipeline = (Vulkan::rs_pipeline_vk*)matVariant->getRsPipeline();
				RenderState oldstate = pipeline->renderState;
				Vulkan::destroyRsPipeline(ctx, pipeline);
				matVariant->mRsPipeline = createVariantPipeline(pass, matVariant->getShaderStageInfo(), oldstate);
			}
		}


	}

	MaterialPass* MaterialTemplate::createMaterialPass(RenderPass* pass, const std::vector<std::pair<ShaderStage, std::string>>& shaderMarco)
	{
		return createMaterialPass(pass, shaderMarco, mRenderState);
	}

	Render::MaterialPass* MaterialTemplate::createMaterialPass(RenderPass* pass, const std::vector<std::pair<ShaderStage, std::string>>& shaderMarco, const RenderState& state)
	{
		auto ctx = RenderSystem::instance()->getRenderContext();

		auto itor = mMaterialPassMap.find(pass->getPassName());
		if (itor != mMaterialPassMap.end()) {
			destroyMaterialPass(itor->second);
		}
		auto pipeline = createVariantPipeline(pass, shaderMarco, state);
		MaterialPass* mat = new MaterialPass(pass, this, pipeline, shaderMarco);

		mMaterialPassMap[pass->getPassName()] = mat;

		return mat;
	}

	MaterialPass* MaterialTemplate::getMaterialPass(const Name& name)
	{
		auto itor = mMaterialPassMap.find(name);
		if (itor != mMaterialPassMap.end()) {
			return itor->second;
		}

		return nullptr;
	}
	MaterialTemplate::~MaterialTemplate()
	{
		OnUnload();
	}

	rs_pipeline* MaterialTemplate::createVariantPipeline(RenderPass* pass,const std::vector<std::pair<ShaderStage, std::string>>& shaderMarco, const RenderState& state)
	{

		auto ctx = RenderSystem::instance()->getRenderContext();
		std::vector<rs_shader_module*> modules;
		ShaderCompileDesc compileDesc{};
		compileDesc.shaderIncludeFindFunc = RenderSystem::instance()->getShaderIncludeSearchFunc();
		compileDesc.shaderIncludeDirectories = RenderSystem::instance()->getShaderIncludeSearchDir();
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
			compileDesc.shaderName = shader;
			compileDesc.generateDebugInfo = true;
			sd.compileDesc = &compileDesc;
			modules.push_back(Vulkan::createRsShader(ctx, sd));
		}

		PipelineDesc desc{
			.type = PipelineType::Graphics,
			.shaders = modules,
			.renderState = state,
			.vertexInputDesc = getInputVertexDesc()
		};

		auto pipeline = Vulkan::createRsPipeline(ctx, (Vulkan::rs_renderpass_vk*)pass->getRaw(), desc);
		for (auto sdm : modules) {
			auto mod = (Vulkan::rs_shader_module_vk*)sdm;
			Vulkan::destroyRsShader(ctx, mod);
		}
		return pipeline;

	}

	void MaterialTemplate::destroyMaterialPass(MaterialPass* mat)
	{
		auto ctx = RenderSystem::instance()->getRenderContext();
		auto pipeline = (Vulkan::rs_pipeline_vk*)mat->getRsPipeline();
		Vulkan::destroyRsPipeline(ctx, pipeline);
		delete mat;
	}

}
