#ifndef MATERIAL_TEMPLATE_MANAGER_H_
#define MATERIAL_TEMPLATE_MANAGER_H_

#include "common/Singleton.h"
#include "render_resource_createinfo.h"
#include "common/Name.h"
#include "Renderer/MaterialTemplate.h" 
#include "common/ResourceManager.h"        

namespace Render {
    class MaterialTemplate;
    class MaterialPass;
    class RenderPass;

    class MaterialTemplateManager :
        public ResourceManager<MaterialTemplate>,
        public Singleton<MaterialTemplateManager>
    {
    public:
        MaterialTemplateManager();
        virtual ~MaterialTemplateManager();

        virtual const Name& typeName() const override;

        MaterialTemplatePtr createMaterialTemplate(
            const Name& templateName,
            const ShaderStageInfo& shaderInfo,
            const RenderState& state,
            const VertexInputDescription& inputDesc
        );

        MaterialTemplatePtr getMaterialTemplate(const Name& templateName)const;

        void broadcastPipelineRebuild(RenderPass* passChanged);

    protected:
        virtual MaterialTemplate* loadImpl(const Name& id) override;
        virtual void unloadImpl(MaterialTemplate* res) override;

    private:
        virtual void createNecessaryPersistenceResources() override {}
    };
};

#endif