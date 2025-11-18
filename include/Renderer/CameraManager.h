#ifndef CAMERA_MANAGER_H_
#define CAMERA_MANAGER_H_

#include <map>
#include <memory>
#include <functional>
#include "common/Singleton.h"
#include "common/Name.h"

namespace Render {
	class Camera;
	class CameraManagerPrivate;
	struct rs_drawdata;

	class CameraManager : public Singleton<CameraManager> {
	public:
		void RegisterCamera(Camera* camera,uint32_t priority);
		void UnregisterCamera(Camera* camera);
		void TraversalCameras(const std::function<void(Camera*)>& func);
		void InitCameraDrawData();
		rs_drawdata* GetCameraDrawData(Camera* camera);
	private:
		std::multimap<uint32_t, Camera*> mPriorityCameras;
		std::unique_ptr<CameraManagerPrivate> mDp;
	};
}

#endif