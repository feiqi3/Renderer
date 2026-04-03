#ifndef FPS_CONTROLLER_COMPONENT_H_
#define FPS_CONTROLLER_COMPONENT_H_
#include "function/Component.h"
#include "function/FPSCameraControll.h"
namespace Render {
	class FPSControllerComponent : public Component {
	public:
		FPSControllerComponent();
		virtual void onEnable();
		virtual void onUpdate(float dt);
		virtual void onTransformChanged();
		void	setCamera(Camera* cam);
		void	setSensitivity(float s);
		void	setSpeed(float s);
		float	getSensitivity()const;
		float	getSpeed()const;
	private:
		class Camera* mCamera = nullptr;
		FPSCameraControl mControl;
		float			mSpeed = 1.f;
		float			mSensitivity = 10.f;
	};
}
#endif