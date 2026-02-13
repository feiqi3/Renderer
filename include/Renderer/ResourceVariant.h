#ifndef RESOURCE_VARIANT_H_
#define RESOURCE_VARIANT_H_
#include <variant>
#include "Renderer/Texture.h" 
#include "Renderer/SamplerResourceManager.h"

namespace Render {

    class RenderResourceVariant {
    public:
        inline RenderResourceVariant() : mData(std::monostate{}) {}

        inline RenderResourceVariant(TexturePtr tex) : mData(tex) {}

        inline RenderResourceVariant(SamplerPtr sampler) : mData(sampler) {}

        inline bool isTexture() const { return std::holds_alternative<TexturePtr>(mData); }
        inline bool isSampler() const { return std::holds_alternative<SamplerPtr>(mData); }
        inline bool isValid() const { return !std::holds_alternative<std::monostate>(mData); }
        inline bool hasResource() const {
            if (isTexture()) {
                return getTexture() != nullptr;
            }
            else if (isSampler()) {
                return getSampler() != nullptr;
            }
            else {
                return false;
            }
        }
        inline TexturePtr getTexture() const { return std::get<TexturePtr>(mData); }
        inline SamplerPtr getSampler() const { return std::get<SamplerPtr>(mData); }

        ~RenderResourceVariant() = default;

    private:
        std::variant<std::monostate, TexturePtr, SamplerPtr> mData;
    };
}

#endif