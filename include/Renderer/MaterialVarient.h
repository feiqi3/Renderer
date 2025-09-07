#ifndef MATERIAL_VARIENT_H
#define MATERIAL_VARIENT_H
#include "render_resource.h"
#include <atomic>
namespace Render {
	class Material {

		inline bool isValid()const {
			return mRsPipeline && mRsRenderPass;
		}

		inline rs_pipeline* getRsPipeline()const { return mRsPipeline; }
		
		inline rs_renderpass* getRsRenderPass()const { return mRsRenderPass; }
		
		inline const std::vector<BindingInfo>& getBindingInfo()const {
			return mRsPipeline->bindingInfo;
		}

	private:
		Material(const std::string& materialTemplateName);
		inline void setMacros(const std::string& str) {
			mMacro = str;
		}
		inline void setPipelineAndRenderPass(rs_pipeline* pipeline, rs_renderpass* renderpass) {
			mRsPipeline = pipeline;
			mRsRenderPass = renderpass;
		}
		inline void setVertexShaderName(const std::string& name) {
			mVsName = name;
		}
		inline void setPixelShaderName(const std::string& name) {
			mPsName = name;
		}
	
	private:

		rs_pipeline* mRsPipeline = 0;
		rs_renderpass* mRsRenderPass = 0;
		std::string mMacro;
		std::string mVsName;
		std::string mPsName;
		std::atomic_int32_t mUsedNum = 0;
	};
}
#endif