#include "Renderer/RenderSystem.h"
#include "Renderer/CameraManager.h"
#include "Renderer/Camera.h"
#include "Renderer/RenderPassManager.h"
#include "render_resource_createinfo.h"
#include "Renderer/RenderEntity.h"
#include "common/CommonMath.h"
namespace Render {

	struct alignas(16) CameraDataCommon {
		mat4 MatView;
		mat4 MatProj;
		mat4 MatViewProj;
		mat4 MatInvView;
		mat4 MatInvProj;
		vec4 CameraPosition;
		vec4 CameraUp;
		vec4 CameraFront;
	};

	void updateCameraDataCommon(Camera* cam,CameraDataCommon& common) {
		common.MatView = cam->getViewMatrix();
		common.MatProj = cam->getProjectionMatrix();
		common.MatViewProj =common.MatInvProj * common.MatView;
		common.MatInvView = inverse(common.MatView);
		common.MatInvProj = inverse(common.MatProj);
		common.CameraPosition = vec4(cam->getPosition(),1.f);
		common.CameraUp =vec4(cam->getUp(),0.f);
		common.CameraFront = vec4(cam->getTarget(), 0.f);
	}

	class CameraManagerPrivate {
	public:
		MaterialTemplate* VirtualCameraTemplate = nullptr;
		Material* MainPassCameraMaterial = nullptr;
		rs_binding_pos CameraCommonDataBindingPos;
	};

	void CameraManager::RegisterCamera(Camera* camera, uint32_t priority)
	{
		if (camera == nullptr) {
			assert(0);
			return;
		}
		mPriorityCameras.insert({ priority,camera });
	}

	void CameraManager::UnregisterCamera(Camera* camera)
	{
		for (auto&& itor = mPriorityCameras.begin();itor != mPriorityCameras.end();) {
			Camera* cam = itor->second;
			if (cam == camera || cam->getName() == camera->getName()) {
				itor = mPriorityCameras.erase(itor);
			}
			else {
				++itor;
			}
		}
	}

	void CameraManager::TraversalCameras(const std::function<bool(Camera*)>& func)
	{
		for (auto&& itor = mPriorityCameras.begin();itor != mPriorityCameras.end();++itor) {
			if (!func(itor->second)) {
				break;
			}
		}
	}

	void CameraManager::InitCameraDrawData()
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
		auto pass = pSys->getRenderPass(Name("MainCam"));
		mDp->VirtualCameraTemplate = new MaterialTemplate(VirtualCameraShaderStageInfo, renderState, vtxIA);
		mDp->MainPassCameraMaterial = mDp->VirtualCameraTemplate->createVarient(pass, {});
		mDp->CameraCommonDataBindingPos = pSys->getBindingPos("CameraCommon", mDp->MainPassCameraMaterial);
	}

	void CameraManager::deactiveCamera(Camera* cam)
	{
		this->TraversalCameras([cam](Camera* icam) {
			if (cam == icam) {
				cam->m_active = false;
				return false;
			}
		});
	}

	void CameraManager::activeCamera(Camera* cam)
	{
		this->TraversalCameras([cam](Camera* icam) {
			if (cam == icam) {
				cam->m_active = true;
				return false;
			}
			});
	}

	void CameraManager::updateAllCamera()
	{
		TraversalCameras([this](Camera* cam) {
			if (cam->getCameraActive()) {
				updateCameraDrawData(cam);
			}
			return true;
		});
	}

	Render::rs_drawdata* CameraManager::getCameraDrawData(Camera* cam)
	{
		return cam->getDrawData();
	}

	rs_drawdata* CameraManager::updateCameraDrawData(Camera* camera)
	{
		if (!camera->mCameraDrawData) {
			camera->mCameraDrawData = RenderSystem::instance()->createDrawData();
		}
		Pass tempPass{};
		tempPass.mDrawData = camera->mCameraDrawData;
		tempPass.mMaterial = mDp->MainPassCameraMaterial;
		CameraDataCommon commonData;
		updateCameraDataCommon(camera, commonData);
		RenderSystem::instance()->updateUniformBufferData(mDp->CameraCommonDataBindingPos, &commonData, sizeof(CameraDataCommon), &tempPass);
		return camera->mCameraDrawData;
	}

}