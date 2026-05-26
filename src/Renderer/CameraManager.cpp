#include "Renderer/RenderSystem.h"
#include "Renderer/CameraManager.h"
#include "Renderer/Camera.h"
#include "Renderer/RenderPassManager.h"
#include "render_resource_createinfo.h"
#include "Renderer/RenderEntity.h"
#include "common/CommonMath.h"
#include "Renderer/MaterialTemplateManager.h"
#include "Renderer/GPUShared/CameraData.h"
#include "Renderer/ConstShaderDataManager.h"
namespace Render {

	CameraManager::CameraManager()
	{
	}

	CameraManager::~CameraManager()
	{
	}

	void CameraManager::RegisterCamera(Camera* camera, uint32_t priority)
	{
		if (camera == nullptr) {
			assert(0);
			return;
		}

		if (mNameCameraMap.find(camera->getName()) != mNameCameraMap.end()) {
			assert(false && "Duplicated camera.");
			return;
		}
		mNameCameraMap.insert({ camera->getName(),camera });

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
		mNameCameraMap.erase(camera->getName());
	}

	void CameraManager::TraversalCameras(const std::function<bool(Camera*)>& func)
	{
		for (auto&& itor = mPriorityCameras.begin();itor != mPriorityCameras.end();++itor) {
			if (!func(itor->second)) {
				break;
			}
		}
	}

	void CameraManager::deactiveCamera(Camera* cam)
	{
		this->TraversalCameras([cam](Camera* icam) {
			if (cam == icam) {
				cam->m_active = false;
				return false;
			}
			return true;
		});
	}

	void CameraManager::activeCamera(Camera* cam)
	{
		this->TraversalCameras([cam](Camera* icam) {
			if (cam == icam) {
				cam->m_active = true;
				return false;
			}
			return true;
			});
	}

	void CameraManager::updateAllCamera(rs_commandbuffer* cmdbuf)
	{
		TraversalCameras([this, cmdbuf](Camera* cam) {
			if (cam->getCameraActive()) {
				auto drawData = ConstShaderDataManager::instance()->updateCameraDrawData(cam);
				RenderSystem::instance()->transitDrawdataResourceState(cmdbuf, PipelineType::Graphics, drawData);
			}
			return true;
		});
	}

	Render::Camera* CameraManager::getCamera(const Name& cameraName)
	{
		auto itor = this->mNameCameraMap.find(cameraName);
		if (itor != mNameCameraMap.end())return itor->second;
		return nullptr;
	}

}