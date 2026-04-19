#ifndef MATERIAL_TEMPLATE_H
#define MATERIAL_TEMPLATE_H

#include <string>
#include <vector>
#include <map>
#include "common/Name.h"
#include "render_resource_createinfo.h"
#include "render_resource.h"
#include "common/ResourceHandler.h"
namespace Render {

    class MaterialPass;
    class RenderPass;
    struct rs_pipeline;

    class MaterialTemplate : public IResource {
    public:
        MaterialTemplate(const ShaderStageInfo& shaderInfo, const RenderState& state, const VertexInputDescription& inputDesc);
        virtual ~MaterialTemplate();

        static const Name& typeName();
        virtual const Name& getTypeName() const override;
        virtual ResourceMemory getMemory() const override;
        virtual void OnUnload() override; 

        const RenderState& getRenderState()const { return mRenderState; }
        const ShaderStageInfo& getShaderStageInfo()const { return mShaderInfo; }
        const VertexInputDescription& getInputVertexDesc()const { return mInputDesc; }

        virtual void onRenderPassRTChangedNeedRebuild(RenderPass* pass);

		MaterialPass* createMaterialPass(RenderPass* pass);
		MaterialPass* createMaterialPass(RenderPass* pass, const StageMacroPairs& shaderMarco);
        MaterialPass* createMaterialPass(RenderPass* pass, const StageMacroPairs& shaderMarco, const RenderState& state);
        MaterialPass* getMaterialPass(const Name& passName);
        inline const std::map<Name, MaterialPass*>& getMaterialMap()const {
            return mMaterialPassMap;
        }

    private:
        rs_pipeline* createVariantPipeline(RenderPass* pass, const StageMacroPairs& shaderMarco, const RenderState& state);
        void destroyMaterialPass(MaterialPass* material);

    private:
        friend class MaterialTemplateManager;
        RenderState mRenderState;
        ShaderStageInfo mShaderInfo;
        VertexInputDescription mInputDesc;
        std::map<Name, MaterialPass*> mMaterialPassMap;
    };
	using MaterialTemplatePtr = ResourceHandle<MaterialTemplate>;
}
#endif