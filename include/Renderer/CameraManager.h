#ifndef CAMERA_MANAGER_H_
#define CAMERA_MANAGER_H_

#include <map>
#include <functional>
#include "common/Singleton.h"
#include "common/Name.h"

namespace Render {
	class Camera;

	class CameraManager : public Singleton<CameraManager> {
	public:
		void RegisterCamera(Camera* camera,uint32_t priority);
		void UnregisterCamera(Camera* camera);
		void TraversalCameras(const std::function<void(Camera*)>& func);
	private:
		std::multimap<uint32_t, Camera*> mPriorityCameras;
	};
}

#endif