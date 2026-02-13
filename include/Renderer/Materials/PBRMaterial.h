#ifndef PBR_MATERIAL_H_
#define PBR_MATERIAL_H_

#include "Renderer/MaterialInstance.h"
#include "Renderer/GPUShared/PBREntity.h"
#include "common/CommonMath.h" 

namespace Render {

    class PBRMaterial : public Material {
    public:
        PBRMaterial(MaterialTemplatePtr templatePtr);
        virtual ~PBRMaterial();

        void setBaseColor(const vec4& color);
        void setBaseColorTexture(TexturePtr tex, rs_sampler* sampler);

        void setMetallic(float metallic);
        void setRoughness(float roughness);
        void setAOStrength(float ao);
        void setMetallicRoughnessTexture(TexturePtr tex, rs_sampler* sampler);
        void setAOTexture(TexturePtr tex, rs_sampler* sampler);

        void setNormalTexture(TexturePtr tex, rs_sampler* sampler);
        void setNormalScale(float scale);

        void setEmissive(const vec3& factor);

        void setTexControl(const vec4& control);

        void updatePBRParams();
        virtual void OnUpdateParam() override;
        virtual void OnUnload() override;

    private:
        void createPBRBuffer();

    private:
        GPUShared::PBRData mPBRData;

        rs_buffer* mPBRBuffer = nullptr;

        bool mIsDirty = true;
    };
}

#endif // PBR_MATERIAL_H_