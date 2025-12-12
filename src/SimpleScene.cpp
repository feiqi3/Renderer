#include "SimpleScene.h"
#include "Renderer/RenderPass/MainCameraPass.h"
#include "common/CommonMath.h"
#include "Renderer/CameraManager.h"

namespace Render {
	SimpleScene::SimpleScene()
	{

		mCam = new Camera(Name("Scene"));
		auto rsys = RenderSystem::instance();
		mCube = new CubeEntity();
		mCube->createPass(Name("MainCameraPass"));
	}
	SimpleScene::~SimpleScene() {
		delete mCube;
	}

	void SimpleScene::updateScene(float deltatime)
	{
		static mat4 matrix(1.0);
		RenderSystem::instance()->setCurrentCamera(mCam);
		matrix = rotate(matrix, deltatime, vec3(1, 1, 1));
		auto pass = mCube->getMaterialTemplate()->getVarient(Name("MainCameraPass"));
		auto bindingPos = RenderSystem::instance()->getBindingPos("ObjectCommon", pass);
		RenderSystem::instance()->updateUniformBufferData(bindingPos, &matrix, sizeof(mat4), mCube->getPass(Name("MainCameraPass")));
		auto rp = (MainCameraPass*)RenderSystem::instance()->getRenderPass(Name("MainCameraPass"));
		rp->addToDrawList(mCube);
	}

}