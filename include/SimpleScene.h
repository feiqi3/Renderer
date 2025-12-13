#ifndef SIMPLE_SCENE_H
#define SIMPLE_SCENE_H
#include "Renderer/Camera.h"
#include "Renderer/ObjectEntity.h"
#include "render_resource.h"
namespace Render {
	class SimpleScene {
	public:
		SimpleScene();
		~SimpleScene();
		void updateScene(float time);
	private:
		Camera* mCam;
		CubeEntity* mCube;
		CubeEntity* mCubeB;
		rs_image* mRtColor = nullptr;
		rs_image* mRtDepth = nullptr;
		rs_rendertarget* mRenderTarget = nullptr;
	};
}

#endif