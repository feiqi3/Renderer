
#include "Renderer/MaterialTemplate.h"
#include "Renderer/MaterialVarient.h"
#include "Renderer/RenderSystem.h"
#include "vulkan/vulkan_render_function.h"
#include "vulkan/vulkan_pipeline.h"
#include "vulkan/vulkan_shader_module.h"
#include "Renderer/RenderPass.h"
#include "platform/FileSystem/FileSystem.h"
#include <cassert>
namespace Render {

	static StageMacroPairs mergeStageMacroPairs(const StageMacroPairs& lhs, const StageMacroPairs& rhs)
	{
		StageMacroPairs result = lhs;

		for (const auto& rhsStagePair : rhs)
		{
			ShaderStage stage = rhsStagePair.first;
			const auto& rhsMacros = rhsStagePair.second;

			auto it = std::find_if(result.begin(), result.end(), [stage](const auto& pair) {
				return pair.first == stage;
				});

			if (it != result.end())
			{
				it->second.insert(it->second.end(), rhsMacros.begin(), rhsMacros.end());
			}
			else
			{
				result.push_back(rhsStagePair);
			}
		}
	}

	MaterialTemplate::MaterialTemplate(const ShaderStageInfo& shaderInfo, const RenderState& state, const VertexInputDescription& inputDesc)
		: mRenderState(state), mShaderInfo(shaderInfo), mInputDesc(inputDesc)
	{
		mState = ResourceLoadState::Loaded;
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
		mState = ResourceLoadState::Unloaded;
	}

	void MaterialTemplate::onRenderPassRTChangedNeedRebuild(RenderPass* pass)
	{
		if (!pass)return;

		auto ctx = RenderSystem::instance()->getRenderContext();
		//Rebuild all variant with target pass.
		for (auto&& [name, matVariant] : this->mMaterialPassMap) {
			if (matVariant->getRenderPass() == pass) {
				//1. destroy old pipeline.
				auto pipeline = (Vulkan::rs_graphic_pipeline_vk*)matVariant->getRsPipeline();
				RenderState oldstate = pipeline->renderState;
				Vulkan::destroyRsPipeline(ctx, pipeline);
				matVariant->mRsPipeline = createVariantPipeline(pass, matVariant->getShaderStageInfo(), oldstate);
			}
		}


	}

	MaterialPass* MaterialTemplate::createMaterialPass(const Name& passName, const StageMacroPairs& shaderMacro)
	{
		return createMaterialPass(passName, shaderMacro, mRenderState);
	}

	Render::MaterialPass* MaterialTemplate::createMaterialPass(const Name& passName, const StageMacroPairs& shaderMacro, const RenderState& state)
	{
		auto ctx = RenderSystem::instance()->getRenderContext();

		auto itor = mMaterialPassMap.find(passName);
		if (itor != mMaterialPassMap.end()) {
			destroyMaterialPass(itor->second);
		}
		auto renderPass = RenderSystem::instance()->getRenderPass(passName);
		auto renderPassMacro = renderPass->getPassStageShaderMacro(passName);
		auto newMacro = mergeStageMacroPairs(renderPassMacro, shaderMacro);
		auto pipeline = createVariantPipeline(renderPass, shaderMacro, state);
		MaterialPass* mat = new MaterialPass(renderPass, this, pipeline, shaderMacro);

		mMaterialPassMap[passName] = mat;

		return mat;
	}

	Render::MaterialPass* MaterialTemplate::createMaterialPass(const Name& passName)
	{
		return createMaterialPass(passName, {});
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

	rs_pipeline* MaterialTemplate::createVariantPipeline(RenderPass* pass,const std::vector<std::pair<ShaderStage, MacroPairs>>& shaderMarco, const RenderState& state)
	{

		auto ctx = RenderSystem::instance()->getRenderContext();
		std::vector<rs_shader_module*> modules;
		ShaderCompileDesc compileDesc{};
		compileDesc.shaderIncludeFindFunc = RenderSystem::instance()->getShaderIncludeSearchFunc();
		compileDesc.shaderIncludeDirectories = RenderSystem::instance()->getShaderIncludeSearchDir();
		ShaderDesc sd{};

		for (auto& [stage, shader] : getShaderStageInfo()) {
			
			auto fileStream = Platform::FileSystem::instance()->openFileStream(shader);
			compileDesc.shaderSrcCode.resize(fileStream->getSize());
			fileStream->read(compileDesc.shaderSrcCode.data(), fileStream->getSize());
			sd.shaderCode = 0;
			sd.codeSizeByte = 0;
			sd.stage = stage;
			compileDesc.generateDebugInfo = true;
			compileDesc.langType = ShaderLang::GLSL;
			MacroPairs macroPairs;
			for (auto& [mstage, mmarco] : shaderMarco) {
				if (mstage == stage) {
					compileDesc.macros = mmarco;
					break;
				}
			}
			if (RenderSystem::instance()->isBindlessEnabled()) {
				compileDesc.macros.push_back({"BINDLESS_ENABLE","1"});
			}

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

		auto pipeline = Vulkan::createRsGraphicPipeline(ctx, (Vulkan::rs_renderpass_vk*)pass->getRaw(), desc);
		for (auto sdm : modules) {
			auto mod = (Vulkan::rs_shader_module_vk*)sdm;
			Vulkan::destroyRsShader(ctx, mod);
		}
		return pipeline;

	}

	void MaterialTemplate::destroyMaterialPass(MaterialPass* mat)
	{
		auto ctx = RenderSystem::instance()->getRenderContext();
		auto pipeline = (Vulkan::rs_graphic_pipeline_vk*)mat->getRsPipeline();
		Vulkan::destroyRsPipeline(ctx, pipeline);
		delete mat;
	}

}
