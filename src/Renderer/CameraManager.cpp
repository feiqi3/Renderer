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

	void CameraManager::updateAllCamera()
	{
		TraversalCameras([this](Camera* cam) {
			if (cam->getCameraActive()) {
				ConstShaderDataManager::instance()->updateCameraDrawData(cam);
			}
			return true;
		});
	}

}