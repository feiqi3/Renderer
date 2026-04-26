#include "Renderer/MaterialInstance.h"
#include "common/ResourceSystem.h"
#include "Renderer/MaterialVarient.h"
#include "Renderer/RenderPass.h"
#include "Renderer/RenderSystem.h"
#include <cassert>
#include <cstring> 

namespace Render {

    const Name& Material::typeName() {
        static const Name name("Material");
        return name;
    }

    Material::Material(const MaterialTemplatePtr& matTplt)
        : m_template(matTplt)
    {
        assert(m_template.get() != nullptr);
        mState = ResourceLoadState::Loaded;
    }

    Material::~Material() {
    }

    const Name& Material::getTypeName() const {
        return typeName();
    }

    ResourceMemory Material::getMemory() const {
        ResourceMemory mem{ 0, 0 };
        mem.cpuMemory = (uint32_t)sizeof(*this);
        return mem;
    }

    void Material::OnUnload() {
    }

    void Material::OnUpdateParam(Pass* pass) {
        uploadUniform(pass);
    }

    void Material::bindParameter(const std::string& paramName, TexturePtr tex, int element) {
        mBindingTable.broadcastParameter(Name(paramName), tex, element);
    }

    void Material::bindParameter(const std::string& paramName, TexturePtr tex, ImageViewKey key, int element)
    {
        mBindingTable.broadcastParameter(Name(paramName), tex,key, element);
    }

    void Material::bindParameter(const std::string& paramName, rs_buffer* buffer, int element) {
        mBindingTable.broadcastParameter(Name(paramName), buffer, element);
    }

    void Material::bindParameter(const std::string& paramName, SamplerPtr sampler, int element) {
        mBindingTable.broadcastParameter(Name(paramName), sampler, element);
    }

    void Material::bindParameter(const std::string& paramName, const void* data, u32 size) {
        mBindingTable.broadcastParameterData(Name(paramName), data, size, 0);
    }


    MaterialPass* Material::getMaterialPass(const Name& name) {
        return m_template->getMaterialPass(name);
    }

    void Material::addMaterialPassToRender(const Name& passName) {
        passNamesToRender.push_back(passName);
        if (this->m_template) {
            auto pass = m_template->getMaterialPass(passName);
            mBindingTable.registerPipeline(passName, pass->getRsPipeline());
        }
    }

    MaterialPass* Material::getMaterialPassToRender(const Name& passName) {
        for (const auto& name : passNamesToRender) {
            if (name == passName) {
                return m_template->getMaterialPass(passName);
            }
        }
        return nullptr;
    }

    void Material::setRenderOrder(u32 order) {
        mRenderOrder = order;
    }

    u32 Material::getRenderOrder() const {
        return mRenderOrder;
    }


    void Material::uploadUniform(Pass* pass) {
        if (!pass)return;
        auto bindingTable = this->mBindingTable.getPipelineBindingTable(pass->getPassName());
        if (bindingTable){
            bindingTable->commit(pass->mMaterial->getRsPipeline(), pass->mDrawData);
        }
    }

} // namespace Render