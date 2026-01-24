#include "Renderer/RenderSystem.h"
#include "Renderer/ConstShaderDataManager.h"
#include "Renderer/MaterialTemplateManager.h"
#include "common/CommonMath.h"
#include "Renderer/RenderEntity.h"
#include "render_resource_createinfo.h"
#include "Renderer/RenderPassManager.h"
#include "Renderer/Camera.h"

#include "Renderer/GPUShared/SceneData.h"
#include "Renderer/GPUShared/CameraData.h"

namespace Render {
	class ConstShaderDataManagerPrivate {
	public:
		MaterialTemplate* VirtualCameraTemplate = nullptr;
		Material* MainPassVirtualMaterial = nullptr;
		rs_binding_pos CameraCommonDataBindingPos;
		rs_binding_pos  SceneCommonDataBindingPos;
	};

	ConstShaderDataManager::ConstShaderDataManager()
	{
		mDp = new ConstShaderDataManagerPrivate;
		createVirtualRenderPass();
	}
	ConstShaderDataManager::~ConstShaderDataManager()
	{
		delete mDp;
		mDp = nullptr;
	}
	void ConstShaderDataManager::createVirtualRenderPass()
	{
		static const char* PathToVirtualPipelineVs = "../shader/VirtualPipeline.vs";
		static const char* PathToVirtualPipelinePs = "../shader/VirtualPipeline.ps";
		ShaderStageInfo VirtualCameraShaderStageInfo{
			{ShaderStage::Vertex, PathToVirtualPipelineVs },
			{ShaderStage::Fragment, PathToVirtualPipelinePs }
		};		
		VertexInputDescription vtxIA{};
		RenderState renderState{};

		RenderSystem* pSys = RenderSystem::instance();
		//1. create a pipeline 
		auto pass = pSys->getRenderPass(Name("VirtualRenderPass"));
		mDp->VirtualCameraTemplate = MaterialTemplateManager::instance()->createMaterialTemplate(Name("VirtualCamera"), VirtualCameraShaderStageInfo, renderState, vtxIA);
		mDp->MainPassVirtualMaterial = mDp->VirtualCameraTemplate->createVariant(pass, {});
		mDp->CameraCommonDataBindingPos = pSys->getBindingPos("CameraCommon", mDp->MainPassVirtualMaterial);
		mDp->SceneCommonDataBindingPos  = pSys->getBindingPos("SceneCommon" , mDp->MainPassVirtualMaterial);
		assert(mDp->CameraCommonDataBindingPos != INVALID_BINDING_POS);
		assert(mDp->SceneCommonDataBindingPos  != INVALID_BINDING_POS);
	}
	rs_drawdata* ConstShaderDataManager::updateCameraDrawData(Camera* camera) {
		auto camDrawData = camera->getDrawData();
		Pass tempPass{};
		tempPass.mDrawData = camDrawData;
		tempPass.mMaterial = mDp->MainPassVirtualMaterial;
		auto gptdata = camera->toGPUData();
		RenderSystem::instance()->updateUniformBufferData(mDp->CameraCommonDataBindingPos, (void*)&gptdata, sizeof(GPUShared::GPUCameraData), &tempPass);
		return camDrawData;
	}

	rs_drawdata* ConstShaderDataManager::updateSceneDrawData(Scene* scene)
	{	
		auto sceneDrawData = scene->getSceneDrawData();
		Pass tempPass{};
		tempPass.mDrawData = sceneDrawData;
		tempPass.mMaterial = mDp->MainPassVirtualMaterial;
		auto& lightMgr = scene->getLightMgr();
		auto& gptdata = lightMgr.updateLightData();
		RenderSystem::instance()->updateUniformBufferData(mDp->SceneCommonDataBindingPos, (void*)&gptdata, sizeof(GPUShared::GPUSceneLightData), &tempPass);
		return sceneDrawData;
	}
}
