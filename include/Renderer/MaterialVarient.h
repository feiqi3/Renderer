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

		inline const ShaderStageInfo& getShaderStageInfo()const { return mShaderMacro; }

		inline rs_pipeline* getRsPipeline()const { return mRsPipeline; }
		
		inline RenderPass* getRenderPass()const { return mRenderPass; }

		inline const std::vector<BindingInfo>& getBindingInfo()const {
			return mRsPipeline->bindingInfo;
		}

		inline const std::optional<BindingInfo> getBindingInfoByName(const std::string& name) {
			auto itor = mBindingTable.find(name);
			if (itor == mBindingTable.end()) {
				return std::nullopt;
			}
			return itor->second;
		}

	private:
		MaterialPass(RenderPass* renderpass, MaterialTemplate* fromTemplate, rs_pipeline* pipeline,const ShaderStageInfo& stageInfo);
	private:
		friend class MaterialTemplate;
		RenderPass* mRenderPass;
		MaterialTemplate* mMaterialTemplate = nullptr;
		rs_pipeline* mRsPipeline = 0;
		ShaderStageInfo mShaderMacro;
		std::map<std::string, BindingInfo> mBindingTable;
	};

	class Pass {
	public:
		MaterialPass* mMaterial;
		rs_drawdata* mDrawData;
	};

}
#endif