#ifndef SIMPLE_SCENE_H
#define SIMPLE_SCENE_H
#include "Renderer/Camera.h"
#include "Renderer/ObjectEntity.h"
namespace Render {
	class SimpleScene {
	public:
		SimpleScene();
		~SimpleScene();
		void setCamera(Camera* camera);
		void updateScene(float time);
	private:
		Camera* mCam;
		CubeEntity* mCube;
	};
}

#endif