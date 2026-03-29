#include "Renderer/Materials/PBRMaterial.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/Texture.h"
#include <cstring> 

namespace Render {

    PBRMaterial::PBRMaterial(MaterialTemplatePtr templatePtr)
        : Material(templatePtr)
    {
        mPBRData.baseCol = vec4(1.0f, 1.0f, 1.0f, 1.0f);
        mPBRData.metalRoughAO = vec4(0.0f, 0.5f, 1.0f, 1.0f);
        mPBRData.emissiveFactor = vec4(0.0f, 0.0f, 0.0f, 0.0f);
        mPBRData.texControl = vec4(0.0f, 0.0f, 0.0f, 0.0f);

        createPBRBuffer();

        this->bindParameter("CBUFFER_pbrData", mPBRBuffer);
        updatePBRParams();
    }

    PBRMaterial::~PBRMaterial()
    {
        OnUnload();
    }

    void PBRMaterial::OnUnload()
    {
        if (mPBRBuffer) {
            auto ctx = RenderSystem::instance()->getRenderContext();
            RenderSystem::instance()->destroyBuffer(mPBRBuffer);
            mPBRBuffer = nullptr;
        }

        Material::OnUnload();
    }

    void PBRMaterial::createPBRBuffer()
    {
        BufferDesc info{};
        info.byteSize = sizeof(GPUShared::PBRData);
        info.bufUsage = BufferType::BufferType_Uniform;

        mPBRBuffer = RenderSystem::instance()->createBuffer(nullptr, 0, info);
    }

    void PBRMaterial::updatePBRParams()
    {
        if (!mIsDirty || !mPBRBuffer) return;

        RenderSystem::instance()->updateBufferData(mPBRBuffer, &mPBRData, sizeof(GPUShared::PBRData),0);
        mIsDirty = false;
    }

    void PBRMaterial::OnUpdateParam(Pass* pass)
    {
        updatePBRParams();
    }

    // ================== Setters ==================

    void PBRMaterial::setBaseColor(const vec4& color) {
        mPBRData.baseCol = color;
        mIsDirty = true;
    }

    void PBRMaterial::setBaseColorTexture(TexturePtr tex, SamplerPtr sampler,bool useUV0) {
        bindParameter("u_baseColorTex", tex);
        bindParameter("u_baseColorSampler", sampler);
        if (!tex) {
            mPBRData.texControl.x = -1.0f;
            return;
        }
        if (useUV0) {
            mPBRData.texControl.x = 0.;
        }
        else {
            mPBRData.texControl.x = 1.;
        }
        mIsDirty = true;
    }

    void PBRMaterial::setMetallic(float metallic) {
        mPBRData.metalRoughAO.x = metallic;
        mIsDirty = true;
    }

    void PBRMaterial::setRoughness(float roughness) {
        mPBRData.metalRoughAO.y = roughness;
        mIsDirty = true;
    }

    void PBRMaterial::setAOStrength(float ao) {
        mPBRData.metalRoughAO.z = ao;
        mIsDirty = true;
    }

    void PBRMaterial::setNormalScale(float scale) {
        mPBRData.metalRoughAO.w = scale;
        mIsDirty = true;
    }

    void PBRMaterial::setMetallicRoughnessTexture(TexturePtr tex, SamplerPtr sampler, bool useUV0) {
        bindParameter("u_metallicRoughnessTex", tex);
        bindParameter("u_metallicRoughnessSampler", sampler);
        if (!tex) {
            mPBRData.texControl.y = -1.0f;
            return;
        }
        if (useUV0) {
            mPBRData.texControl.y = 0.0f;
        }
        else {
            mPBRData.texControl.y = 1.0f;
        }
        mIsDirty = true;
    }

    void PBRMaterial::setNormalTexture(TexturePtr tex, SamplerPtr sampler, bool useUV0) {
        bindParameter("u_normalTex", tex);
        bindParameter("u_normalSampler", sampler);
        if (!tex) {
            mPBRData.texControl.z = -1.f;
            return;
        }
        if (useUV0) {
            mPBRData.texControl.z = 0.0f;
        }
        else {
            mPBRData.texControl.z = 1.0f;
        }
        mIsDirty = true;
    }

    void PBRMaterial::setAOTexture(TexturePtr tex, SamplerPtr sampler, bool useUV0) {
        bindParameter("u_AOTex", tex);
        bindParameter("u_AOSampler", sampler);
        if (!tex) {
            mPBRData.texControl.w = -1.0f;
            return;
        }
        if (useUV0) {
            mPBRData.texControl.w = 0.0f;
        }
        else {
            mPBRData.texControl.w = 1.0f;
        }
        mIsDirty = true;
    }

    void PBRMaterial::setEmissive(const vec3& factor) {
        mPBRData.emissiveFactor = vec4(factor, mPBRData.emissiveFactor.w);
        mIsDirty = true;
    }

    void PBRMaterial::setTexControl(const vec4& control) {
        mPBRData.texControl = control;
        mIsDirty = true;
    }
}