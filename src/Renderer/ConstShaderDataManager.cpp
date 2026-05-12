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
		MaterialTemplatePtr VirtualCameraTemplate = nullptr;
		MaterialPass* MainPassVirtualMaterial = nullptr;
		rs_binding_pos CameraCommonDataBindingPos;
		rs_binding_pos SceneCommonDataBindingPos;
		rs_binding_pos ObjectCommonBindingPos;
		rs_binding_pos PrefilterEnvMapBindingPos;
		rs_binding_pos BRDFLutBindingPos;
		rs_binding_pos EnvMapSamplerBIndingPos;
		rs_binding_pos GlobalBindlessUAVBuffersBindingPos;
		rs_binding_pos GlobalBindlessUAVImagesBindingPos;
		rs_binding_pos GlobalBindlessSamplersBindingPos;
		rs_binding_pos GlobalBindlessTexturesBindingPos;
		rs_bindless_data* mGlobalBindlessDrawData = nullptr;
	};

	ConstShaderDataManager::ConstShaderDataManager()
	{
		mDp = new ConstShaderDataManagerPrivate;
		createVirtualRenderPass();
	}
	ConstShaderDataManager::~ConstShaderDataManager()
	{
		RenderSystem::instance()->setGlobalBindlessData(nullptr);
		RenderSystem::instance()->destroyBindlessData(mDp->mGlobalBindlessDrawData);
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
		mDp->MainPassVirtualMaterial = mDp->VirtualCameraTemplate->createMaterialPass(pass, {});
		mDp->CameraCommonDataBindingPos = pSys->getBindingPos("CameraCommon", mDp->MainPassVirtualMaterial);
		
		//TODO: FIX ME
		mDp->SceneCommonDataBindingPos  = pSys->getBindingPos("SceneCommon" , mDp->MainPassVirtualMaterial);
		
		mDp->ObjectCommonBindingPos		= pSys->getBindingPos("ObjData", mDp->MainPassVirtualMaterial);
		assert(mDp->CameraCommonDataBindingPos	!= INVALID_BINDING_POS);
		assert(mDp->SceneCommonDataBindingPos	!= INVALID_BINDING_POS);
		assert(mDp->ObjectCommonBindingPos		!= INVALID_BINDING_POS);

		mDp->PrefilterEnvMapBindingPos  = pSys->getBindingPos("PreFilterEnvMap", mDp->MainPassVirtualMaterial);
		mDp->BRDFLutBindingPos			= pSys->getBindingPos("BRDFLut", mDp->MainPassVirtualMaterial);
		mDp->EnvMapSamplerBIndingPos	= pSys->getBindingPos("SceneTextureSampler", mDp->MainPassVirtualMaterial);
		
		//Enable bindless related
		mDp->GlobalBindlessUAVBuffersBindingPos = pSys->getBindingPos("GlobalUAVBuffers", mDp->MainPassVirtualMaterial);
		mDp->GlobalBindlessUAVImagesBindingPos = pSys->getBindingPos("GlobalUAVImages", mDp->MainPassVirtualMaterial);
		mDp->GlobalBindlessSamplersBindingPos = pSys->getBindingPos("GlobalSamplers", mDp->MainPassVirtualMaterial);
		mDp->GlobalBindlessTexturesBindingPos = pSys->getBindingPos("GlobalTextures", mDp->MainPassVirtualMaterial);
		if (RenderSystem::instance()->isBindlessEnabled()) {
			auto bindlessData = RenderSystem::instance()->createBindlessData(mDp->MainPassVirtualMaterial->getRsPipeline());
			mDp->mGlobalBindlessDrawData = bindlessData;
			bindlessData->storageBindlessPos = mDp->GlobalBindlessUAVImagesBindingPos;
			bindlessData->samplerBindlessPos = mDp->GlobalBindlessSamplersBindingPos;
			bindlessData->textureBindlessPos = mDp->GlobalBindlessTexturesBindingPos;
			RenderSystem::instance()->setGlobalBindlessData(bindlessData);
		}


		assert(mDp->PrefilterEnvMapBindingPos != INVALID_BINDING_POS);
		assert(mDp->BRDFLutBindingPos != INVALID_BINDING_POS);
		assert(mDp->EnvMapSamplerBIndingPos != INVALID_BINDING_POS);
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
		if (!scene)return nullptr;
		auto sceneDrawData = scene->getSceneDrawData();
		Pass tempPass{};
		tempPass.mDrawData = sceneDrawData;
		tempPass.mMaterial = mDp->MainPassVirtualMaterial;
		auto& lightMgr = scene->getLightMgr();
		auto& gptdata = lightMgr.updateLightData();
		RenderSystem::instance()->updateUniformBufferData(mDp->SceneCommonDataBindingPos, (void*)&gptdata, sizeof(GPUShared::GPUSceneLightData), &tempPass);

		if (lightMgr.getPrefilterEnvMap()) {
			RenderSystem::instance()->updateUniform(mDp->PrefilterEnvMapBindingPos,0, lightMgr.getPrefilterEnvMap()->getRsImage(), &tempPass);
		}

		if (lightMgr.getBRDFLut()) {
			RenderSystem::instance()->updateUniform(mDp->BRDFLutBindingPos, 0, lightMgr.getBRDFLut()->getRsImage(), &tempPass);
		}

		if (lightMgr.getIBLSampler()) {
			RenderSystem::instance()->updateUniform(mDp->EnvMapSamplerBIndingPos, 0, lightMgr.getIBLSampler()->getRsSampler(), &tempPass);
		}
		return sceneDrawData;
	}

	Render::rs_binding_pos ConstShaderDataManager::getObjectCommonDataBindingPos()
	{
		return mDp->ObjectCommonBindingPos;
	}

	Render::rs_binding_pos ConstShaderDataManager::getSceneCommonDataBindingPos()
	{
		return mDp->SceneCommonDataBindingPos;
	}

	Render::rs_binding_pos ConstShaderDataManager::getCameraCommonDataBindngPos()
	{
		return mDp->CameraCommonDataBindingPos;
	}

	Render::rs_binding_pos ConstShaderDataManager::getGlobalBindlessUAVBuffersBindingPos()
	{
		return mDp->GlobalBindlessUAVBuffersBindingPos;
	}

	Render::rs_binding_pos ConstShaderDataManager::getGlobalBindlessUAVImagesBindingPos()
	{
		return mDp->GlobalBindlessUAVImagesBindingPos;
	}

	Render::rs_binding_pos ConstShaderDataManager::getGlobalBindlessSamplersBindingPos()
	{
		return mDp->GlobalBindlessSamplersBindingPos;
	}

	Render::rs_binding_pos ConstShaderDataManager::getGlobalBindlessTexturesBindingPos()
	{
		return mDp->GlobalBindlessTexturesBindingPos;
	}

}
