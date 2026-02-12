#include "Renderer/MaterialInstance.h"
#include <assert.h>

namespace Render {

    std::optional<Material::_ParameterPair&> Render::Material::getParameterInfo(const std::string& paramName)
    {
        auto itor = mParameterMap.find(paramName);
        if (itor == mParameterMap.end()) {
            for (const auto& [name, pass] : m_template->getMaterialMap()) {
                auto bindingInfoOption = pass->getBindingInfoByName(paramName);
                if (bindingInfoOption.has_value()) {
                    _ParameterPair bpair{};
                    bpair.bindingPos = bindingInfoOption->bindingPos;
                    bpair.parameterType = bindingInfoOption->type;
                    auto ret = this->mParameterMap.insert({ paramName, bpair });
                    return (ret.first)->second;
                }
            }
        }
        else {
            return itor->second;
        }
        return std::nullopt;
    }

    Material::Material(MaterialTemplate* matTplt)
    {
		assert(matTplt != nullptr);
        m_template = matTplt;
    }

    Material::~Material()
    {
    }

    void Material::bindParameter(const std::string& paramName, TexturePtr tex)
    {
        auto bpair = this->getParameterInfo(paramName);
        if (bpair.has_value()) {
            if (bpair->get().parameterType == UniformType::Texture ||
                bpair->get().parameterType == UniformType::StorageImage ||
                bpair->get().parameterType == UniformType::InputAttachment) {
                bpair->get().texture = tex;
            }
            else {
                assert(false && "Parameter type mismatch: Expected a Texture type.");
            }
        }
    }

    // 绑定 Buffer (StorageBuffer 或 ConstantBuffer)
    void Material::bindParameter(const std::string& paramName, rs_buffer* buffer)
    {
        auto bpair = this->getParameterInfo(paramName);
        if (bpair.has_value()) {
            UniformType type = bpair->get().parameterType;

            if (type == UniformType::ConstantBuffer || type == UniformType::StorageBuffer) {
                bpair->get().rawPtr = static_cast<void*>(buffer);
            }
            else if (type == UniformType::UniformBuffer) {
                assert(false && "UniformBuffer is not allowed to be set via Material class.");
            }
            else {
                assert(false && "Parameter type mismatch: Expected a Buffer type.");
            }
        }
    }

    void Material::bindParameter(const std::string& paramName, rs_sampler* sampler)
    {
        auto bpair = this->getParameterInfo(paramName);
        if (bpair.has_value()) {
            if (bpair->get().parameterType == UniformType::Sampler) {
                bpair->get().rawPtr = static_cast<void*>(sampler);
            }
            else {
                assert(false && "Parameter type mismatch: Expected a Sampler.");
            }
        }
    }
    void Material::uploadUniform(Pass* pass)
    {

        for (auto& [name, bpair] : mParameterMap) {
            if (bpair.rawPtr == nullptr && bpair.rawPtr == nullptr) {
                continue;
            }
            auto sys = RenderSystem::instance();
            switch (bpair.parameterType) {
            case UniformType::StorageBuffer:
            case UniformType::ConstantBuffer:
                sys->updateUniform(bpair.bindingPos, (rs_sampler*)bpair.rawPtr, pass);
                break;

            case UniformType::UniformBuffer:
                assert(false && "Not supported here");
                break;

            case UniformType::StorageImage:
            case UniformType::Texture:
            case UniformType::InputAttachment:
                sys->updateUniform(bpair.bindingPos, bpair.texture->getRsImage(), pass);
                break;

            case UniformType::Sampler:
                sys->updateUniform(bpair.bindingPos, (rs_sampler*)bpair.rawPtr, pass);
                break;

            case UniformType::Count:
            default:
                assert(false && "Invalid UniformType encountered!");
                break;
            }

        }

    }
}