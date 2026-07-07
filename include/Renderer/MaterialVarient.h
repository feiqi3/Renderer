#ifndef MATERIAL_VARIENT_H
#define MATERIAL_VARIENT_H
#include "render_resource.h"
#include "MaterialTemplate.h"
#include <atomic>
#include <optional>

namespace Render {
	class RenderPass;
	class MaterialPass {
	public:
		inline bool isValid()const {
			return mRsPipeline && mRenderPass;
		}

		inline const StageMacroPairs& getShaderStageInfo()const { return mShaderMacro; }

		inline rs_pipeline* getRsPipeline()const { return mRsPipeline; }
		
		inline RenderPass* getRenderPass()const { return mRenderPass; }

		inline const Name& getMaterialPassName()const { return mLogicPassName; }

		inline const std::vector<ResourceLocation>& getResourceLocations()const {
			return mRsPipeline->resources;
		}

		inline const std::optional<ResourceLocation> getBindingInfoByName(const Name& name) {
			for (const auto& resource : mRsPipeline->resources) {
				if (resource.itemName == name) {
					return resource;
				}
			}
			return std::nullopt;
		}

	private:
		MaterialPass(RenderPass* renderpass,const Name& logicPassName, MaterialTemplate* fromTemplate, rs_pipeline* pipeline,const StageMacroPairs& stageInfo);
	private:
		friend class MaterialTemplate;
		Name		mLogicPassName;
		RenderPass* mRenderPass;
		MaterialTemplate* mMaterialTemplate = nullptr;
		rs_pipeline* mRsPipeline = 0;
		StageMacroPairs mShaderMacro;
	};

	class Pass {
	public:
		const Name& getPassName()const;
		MaterialPass* mMaterialPass;
		rs_drawdata* mDrawData;
	};

}
#endif