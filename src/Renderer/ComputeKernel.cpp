#include "Renderer/ComputeKernel.h"
#include "Renderer/RenderSystem.h"
#include "vulkan/vulkan_render_function.h"
#include "vulkan/vulkan_pipeline.h"
#include "vulkan/vulkan_shader_module.h"
#include <stdio.h>
#include <cassert>

namespace Render {

    ComputeKernel::ComputeKernel(const std::string& shaderPath, const std::string& marco) {
        auto ctx = RenderSystem::instance()->getRenderContext();
        auto sys = RenderSystem::instance();

        FILE* File = fopen(shaderPath.c_str(), "r");
        if (!File) {
            assert(0 && "Compute shader file not found!");
            return;
        }
        fseek(File, 0, SEEK_END);
        uint32_t size = ftell(File);
        rewind(File);

        std::string shaderCode;
        shaderCode.resize(size);
        fread(shaderCode.data(), 1, size, File);
        fclose(File);

        ShaderDesc sd{};
        ShaderCompileDesc compileDesc{};
        compileDesc.generateDebugInfo = true;
        compileDesc.langType = ShaderLang::GLSL;
        compileDesc.shaderSrcCode = marco + shaderCode;
        compileDesc.stage = ShaderStage::Compute;
        compileDesc.shaderName = shaderPath;

        sd.shaderCode = 0;
        sd.codeSizeByte = 0;
        sd.stage = ShaderStage::Compute;
        sd.compileDesc = &compileDesc;

        auto shaderModule = (rs_shader_module*)Vulkan::createRsShader(ctx, sd);

        mPipeline = sys->createComputePipeline(shaderModule);
        if (mPipeline) {
            rs_pipeline* basePipeline = reinterpret_cast<rs_pipeline*>(mPipeline);
            for (auto&& binding : basePipeline->pipelineLayout->bindingInfo) {
                mBindingTable.insert({ binding.bindingItemName.str(), binding});
            }
        }
    }

    ComputeKernel::~ComputeKernel() {
        auto sys = RenderSystem::instance();
        if (mCurrentDrawData) {
            sys->destroyDrawData(mCurrentDrawData);
            mCurrentDrawData = nullptr;
        }
        if (mPipeline) {
            sys->destroyPipeline(mPipeline);
            mPipeline = nullptr;
        }
    }

    std::optional<BindingInfo> ComputeKernel::getBindingInfoByName(const std::string& name) const {
        auto itor = mBindingTable.find(name);
        if (itor == mBindingTable.end()) {
            return std::nullopt;
        }
        return itor->second;
    }

    void ComputeKernel::setParameter(const std::string& name, rs_buffer* buffer) {
        mPendingParams[name] = RenderResourceVariant(buffer);
    }

    void ComputeKernel::setParameter(const std::string& name, TexturePtr texture) {
        mPendingParams[name] = RenderResourceVariant(texture);
    }

    void ComputeKernel::setParameter(const std::string& name, SamplerPtr sampler) {
        mPendingParams[name] = RenderResourceVariant(sampler);
    }

    void ComputeKernel::setParameter(const std::string& name, const void* data, uint32_t size) {
        RenderResourceVariant var;
        var.setUniformBuffer(data, size); 
        mPendingParams[name] = std::move(var);
    }

    void ComputeKernel::dispatch(rs_commandbuffer* cmd, uint32_t groupX, uint32_t groupY, uint32_t groupZ) {
        if (!mPipeline) return;
        auto sys = RenderSystem::instance();
        rs_pipeline* basePipeline = reinterpret_cast<rs_pipeline*>(mPipeline);

        if (mCurrentDrawData) {
            sys->destroyDrawData(mCurrentDrawData);
        }
        mCurrentDrawData = sys->createDrawData();

        for (auto& [name, variant] : mPendingParams) {
            auto infoOpt = getBindingInfoByName(name);
            if (!infoOpt) continue;

            if (variant.isRsBuffer()) {
                sys->updateUniform(infoOpt->bindingPos,0, variant.getRsBuffer(), basePipeline, mCurrentDrawData);
            }
            else if (variant.isTexture()) {
                sys->updateUniform(infoOpt->bindingPos,0, variant.getTexture()->getRsImage(), basePipeline, mCurrentDrawData);
            }
            else if (variant.isSampler()) {
                sys->updateUniform(infoOpt->bindingPos,0, variant.getSampler()->getRsSampler(), basePipeline, mCurrentDrawData);
            }
            else if (variant.isUniformBuffer()) {
                void* dataPtr = nullptr;
                uint32_t size = 0;
                variant.getData(&dataPtr, &size);
                sys->updateUniformBufferData(infoOpt->bindingPos, dataPtr, size, basePipeline, mCurrentDrawData);
            }
        }
		sys->dispatchCompute(cmd, mPipeline, mCurrentDrawData, groupX, groupY, groupZ);
    }
}