#include "Renderer/CameraManager.h"
#include "Renderer/Camera.h"
void Render::CameraManager::RegisterCamera(Camera* camera, uint32_t priority)
{
	if (camera == nullptr) {
		assert(0);
		return;
	}
	mPriorityCameras.insert({ priority,camera });
}

void Render::CameraManager::UnregisterCamera(Camera* camera)
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

void Render::CameraManager::TraversalCameras(const std::function<void(Camera*)>& func)
{
	for (auto&& itor = mPriorityCameras.begin();itor != mPriorityCameras.end();++itor) {
		func(itor->second);
	}
}

