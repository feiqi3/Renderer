#include "Renderer/ComputeKernel.h"
#include "Renderer/RenderSystem.h"
#include "vulkan/vulkan_render_function.h"
#include "vulkan/vulkan_pipeline.h"
#include "vulkan/vulkan_shader_module.h"
#include "platform/FileSystem/FileSystem.h"
#include <stdio.h>
#include <cassert>

namespace Render {

    ComputeKernel::ComputeKernel(const std::string& shaderPath, const MacroPairs& macros) {
        auto ctx = RenderSystem::instance()->getRenderContext();
        auto sys = RenderSystem::instance();
        std::string shaderCode;
        {
            auto streamPtr = Platform::FileSystem::instance()->openFileStream(shaderPath);
            if (!streamPtr) {
                assert(0 && "Compute shader file not found!");
                return;
            }

            shaderCode.resize(streamPtr->getSize());
            streamPtr->read(shaderCode.data(), streamPtr->getSize());
        }

        ShaderDesc sd{};
        ShaderCompileDesc compileDesc{};
        compileDesc.generateDebugInfo = true;
        compileDesc.langType = ShaderLang::GLSL;
        compileDesc.shaderSrcCode = shaderCode;
        compileDesc.stage = ShaderStage::Compute;
        compileDesc.shaderName = shaderPath;
        compileDesc.shaderIncludeFindFunc = RenderSystem::instance()->getShaderIncludeSearchFunc();
        compileDesc.shaderIncludeDirectories = RenderSystem::instance()->getShaderIncludeSearchDir();
        compileDesc.generateDebugInfo = true;
        compileDesc.macros = macros;
        sd.shaderCode = 0;
        sd.codeSizeByte = 0;
        sd.stage = ShaderStage::Compute;
        sd.compileDesc = &compileDesc;

        auto shaderModule = (rs_shader_module*)Vulkan::createRsShader(ctx, sd);

        mPipeline = sys->createComputePipeline(shaderModule);

        if (mPipeline) {
            mBindingTable.init(Name("ComputePass"), mPipeline);
        }
        else {
            assert(false);
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


    void ComputeKernel::setParameter(const std::string& name, rs_buffer* buffer, int element) {
        this->mBindingTable.updateParameter(Name(name), buffer,0 ,0, element);
    }

    void ComputeKernel::setParameter(const std::string& name, rs_buffer* buffer, uint32_t offset, uint32_t size, int element)
    {
        this->mBindingTable.updateParameter(Name(name), buffer,offset,size, element);
    }

    void ComputeKernel::setParameter(const std::string& name, TexturePtr texture, int element) {
        this->mBindingTable.updateParameter(Name(name), texture, element);
    }

    void ComputeKernel::setParameter(const std::string& name, TexturePtr texture, ImageViewKey key, int element)
    {
        this->mBindingTable.updateParameter(Name(name), texture, key, element);
    }

    void ComputeKernel::setParameter(const std::string& name, SamplerPtr sampler, int element) {
        this->mBindingTable.updateParameter(Name(name), sampler, element);
    }

    void ComputeKernel::setParameter(const std::string& name, const void* data, uint32_t size) {
        this->mBindingTable.updateParameterData(Name(name), data, size);
    }

    void ComputeKernel::dispatch(rs_commandbuffer* cmd, uint32_t groupX, uint32_t groupY, uint32_t groupZ) {
        if (!mPipeline) return;
        auto sys = RenderSystem::instance();
        rs_pipeline* basePipeline = reinterpret_cast<rs_pipeline*>(mPipeline);

        if (mCurrentDrawData) {
            sys->destroyDrawData(mCurrentDrawData);
        }
        mCurrentDrawData = sys->createDrawData();

        mBindingTable.commit(mPipeline, mCurrentDrawData);

		sys->dispatchCompute(cmd, mPipeline, mCurrentDrawData, groupX, groupY, groupZ);
    }
}