#include "Renderer/MaterialTemplateManager.h"
#include "Renderer/MaterialTemplate.h"
#include "Renderer/RenderPass.h"
#include "function/EngineResourceManager.h"
#include "common/ResourceSystem.h"
#include <exception>
#include <stdexcept>
#include <cassert>
namespace Render {

    MaterialTemplateManager::MaterialTemplateManager()
    {
    }

    MaterialTemplateManager::~MaterialTemplateManager()
    {
        clearAll();
    }

    const Name& MaterialTemplateManager::typeName() const {
        return MaterialTemplate::typeName();
    }

    MaterialTemplatePtr MaterialTemplateManager::createMaterialTemplate(
        const Name& templateName,
        const ShaderStageInfo& shaderInfo,
        const RenderState& state,
        const VertexInputDescription& inputDesc)
    {
        auto* entry = this->acquire(templateName);
        if (entry) {
            assert(0 && "Error: duplicated material template name");
            throw std::runtime_error("Error: duplicated material template name");
            return nullptr;
        }

        auto mt = new MaterialTemplate(shaderInfo, state, inputDesc);

        auto* newEntry = this->registerResource(
            templateName,
            mt,
            ResourceLifetime::Transient,nullptr
        );

        if (!newEntry) {
            delete mt;
            return nullptr;
        }
        return ResourceHandle <MaterialTemplate>(this,newEntry);
    }

    MaterialTemplatePtr MaterialTemplateManager::getMaterialTemplate(const Name& templateName) const
    {
        return ResourceSystem::instance()->getResource<MaterialTemplate>(typeName(), templateName);
    }

    MaterialTemplate* MaterialTemplateManager::loadImpl(const Name& id)
    {
        throw std::runtime_error("Not implemented");
        return nullptr;
    }

    void MaterialTemplateManager::unloadImpl(MaterialTemplate* res)
    {
        delete res;
    }

    void MaterialTemplateManager::broadcastPipelineRebuild(RenderPass* passChanged)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& [name, entry] : m_entries) {
            if (entry && entry->resource) {
                auto* mat = static_cast<MaterialTemplate*>(entry->resource);
                if (mat->IsReady()) {
                    mat->onRenderPassRTChangedNeedRebuild(passChanged);
                }
            }
        }
    }
}