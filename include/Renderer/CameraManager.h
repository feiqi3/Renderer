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
		CameraManager();
		~CameraManager();
		void RegisterCamera(Camera* camera,uint32_t priority);
		void UnregisterCamera(Camera* camera);
		void TraversalCameras(const std::function<bool(Camera*)>& func);
		void InitCameraDrawData();
		void deactiveCamera(Camera* cam);
		void activeCamera(Camera* cam);
		void updateAllCamera();
		rs_drawdata* getCameraDrawData(Camera* cam);
		friend class RenderSystem;
	private:
		rs_drawdata* updateCameraDrawData(Camera* camera);
	private:
		std::multimap<uint32_t, Camera*> mPriorityCameras;
		std::unique_ptr<CameraManagerPrivate> mDp;
	};
}

#endif