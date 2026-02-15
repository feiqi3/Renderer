#include "Renderer/MaterialInstance.h"
#include "common/ResourceSystem.h"
#include <assert.h>

namespace Render {
    const Name& Material::typeName() {
        static const Name name("Material");
        return name;
    }
    Material::Material(const MaterialTemplatePtr& matTplt)
    {
        assert(matTplt.get() != nullptr);
        m_template = matTplt;
        mState = ResourceState::Loaded;
    }

    Material::~Material()
    {
        OnUnload();
    }

    const Name& Material::getTypeName() const {
        return typeName();
    }

    ResourceMemory Material::getMemory() const {
        ResourceMemory mem{ 0, 0 };
        mem.cpuMemory = (uint32_t)sizeof(*this);
        mem.cpuMemory += (uint32_t)(mParameterMap.size() * sizeof(std::pair<std::string, _ParameterPair>));
        return mem;
    }

    void Material::OnUnload() {
        mParameterMap.clear();
        mState = ResourceState::Unloaded;
    }

    void Material::OnUpdateParam()
    {
    }

    std::optional<Material::_ParameterPair*> Render::Material::getParameterInfo(const std::string& paramName)
    {
        auto itor = mParameterMap.find(paramName);
        if (itor == mParameterMap.end()) {
            for (const auto& [name, pass] : m_template->getMaterialMap()) {
                auto bindingInfoOption = pass->getBindingInfoByName(paramName);
                if (bindingInfoOption.has_value()) {
                    _ParameterPair bpair{};
                    bpair.bindingPos = bindingInfoOption->bindingPos;
                    bpair.parameterType = bindingInfoOption->type;
                    bpair.rawPtr = nullptr;
                    auto ret = this->mParameterMap.insert({ paramName, bpair });
                    return &(ret.first)->second;
                }
            }
        }
        else {
            return &itor->second;
        }
        return std::nullopt;
    }

    void Material::bindParameter(const std::string& paramName, TexturePtr tex)
    {
        auto bpair = this->getParameterInfo(paramName);
        if (bpair.has_value()) {
            auto& pair = *bpair.value();
            if (pair.parameterType == UniformType::Texture ||
                pair.parameterType == UniformType::StorageImage ||
                pair.parameterType == UniformType::InputAttachment) {
                if (tex != nullptr) {
                    pair.var = tex;
                }
                else {
					pair.var = ResourceSystem::instance()->getDefaultResource<Texture>(ResourceName::Texture);
                }
            }
        }
    }

    void Material::bindParameter(const std::string& paramName, rs_buffer* buffer)
    {
        auto bpair = this->getParameterInfo(paramName);
        if (bpair.has_value()) {
            auto& pair = *bpair.value();
            if (pair.parameterType == UniformType::ConstantBuffer ||
                pair.parameterType == UniformType::StorageBuffer) {
                pair.rawPtr = buffer;
            }
        }
    }

    void Material::bindParameter(const std::string& paramName, SamplerPtr sampler)
    {
        auto bpair = this->getParameterInfo(paramName);
        if (bpair.has_value()) {
            auto& pair = *bpair.value();
            if (pair.parameterType == UniformType::Sampler) {
                pair.var = sampler;
            }
        }
    }
    void Material::uploadUniform(Pass* pass)
    {
        this->OnUpdateParam();
        for (auto& [name, bpair] : mParameterMap) {
            if (bpair.rawPtr == nullptr && !bpair.var.hasResource()) {
                continue;
            }
            auto sys = RenderSystem::instance();
            switch (bpair.parameterType) {
            case UniformType::StorageBuffer:
            case UniformType::ConstantBuffer:
                sys->updateUniform(bpair.bindingPos, (rs_buffer*)bpair.rawPtr, pass);
                break;

            case UniformType::UniformBuffer:
                assert(false && "Not supported here");
                break;

            case UniformType::StorageImage:
            case UniformType::Texture:
            case UniformType::InputAttachment:
                sys->updateUniform(bpair.bindingPos, bpair.var.getTexture()->getRsImage(), pass);
                break;

            case UniformType::Sampler:
                sys->updateUniform(bpair.bindingPos, bpair.var.getSampler()->getRsSampler(), pass);
                break;

            case UniformType::Count:
            default:
                assert(false && "Invalid UniformType encountered!");
                break;
            }

        }

    }
    MaterialPass* Material::getMaterialPass(const Name& name)
    {
        return m_template->getMaterialPass(name);
    }

    void Material::addMaterialPassToRender(const Name& passName)
    {
		this->passNamesToRender.push_back(passName);
    }
    void Material::setRenderOrder(u32 order)
    {
        mRenderOrder = order;
    }
    u32 Material::getRenderOrder() const
    {
        return u32();
    }
}