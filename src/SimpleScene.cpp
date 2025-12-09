#include "SimpleScene.h"
#include "Renderer/MainCameraRenderPass.h"
#include "common/CommonMath.h"
namespace Render {
	SimpleScene::SimpleScene()
	{
		mCube = new CubeEntity();
		mCube->createPass(Name("MainPass"));
	}
	SimpleScene::~SimpleScene() {
		delete mCube;
	}

	void SimpleScene::updateScene(float deltatime)
	{
		static mat4 matrix{};
		matrix = rotate(matrix, deltatime, vec3(1, 1, 1));
		auto pass = mCube->getMaterialTemplate()->getVarient(Name("MainPass"));
		auto bindingPos = RenderSystem::instance()->getBindingPos("ObjectCommon", pass);
		RenderSystem::instance()->setCurrentCamera(mCam);
		RenderSystem::instance()->updateUniformBufferData(bindingPos, &matrix, sizeof(mat4), mCube->getPass(Name("MainPass")));
		auto rp = (MainCamPass*)RenderSystem::instance()->getRenderPass(Name("MainPass"));
		rp->addToDraw(mCube);
	}
}