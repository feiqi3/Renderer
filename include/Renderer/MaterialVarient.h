#ifndef MATERIAL_VARIENT_H
#define MATERIAL_VARIENT_H
#include "render_resource.h"
#include "MaterialTemplate.h"
#include <atomic>
#include <optional>

namespace Render {
	class Material {
	public:
		inline bool isValid()const {
			return mRsPipeline && mRsRenderPass;
		}

		inline rs_pipeline* getRsPipeline()const { return mRsPipeline; }
		
		inline rs_renderpass* getRsRenderPass()const { return mRsRenderPass; }
		
		inline const std::vector<BindingInfo>& getBindingInfo()const {
			return mRsPipeline->bindingInfo;
		}

		inline const std::optional<BindingInfo> getBindinginfoByName(const std::string& name) {
			auto itor = mBindingTable.find(name);
			if (itor == mBindingTable.end()) {
				return std::nullopt;
			}
			return itor->second;
		}

	private:
		Material() {}
		inline void setPipelineAndRenderPass(rs_pipeline* pipeline, rs_renderpass* renderpass) {
			mRsPipeline = pipeline;
			mRsRenderPass = renderpass;
			//Construct a binding table
			for (auto&& binding : mRsPipeline->bindingInfo) {
				mBindingTable.insert({ binding.bindingItemName,binding });
			}
		}
	
	private:
		friend class MaterialTemplate;
		rs_pipeline* mRsPipeline = 0;
		rs_renderpass* mRsRenderPass = 0;
		std::atomic_int32_t mUsedNum = 0;
		std::map<std::string, BindingInfo> mBindingTable;
	};

	class Pass {
	public:
		Material* mMaterial;
		rs_drawdata* mDrawData;
	};

}
#endif