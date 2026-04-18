#include "SimpleScene.h"
#include "Renderer/RenderPass/MainCameraPass.h"
#include "common/CommonMath.h"
#include "Renderer/CameraManager.h"
#include "common/ResourceSystem.h"
#include "Renderer/Texture.h"
#include "Renderer/RenderQueue.h"
#include "Renderer/EnginePass.h"
namespace Render {
	SimpleScene::SimpleScene()
	{
		texture = ResourceSystem::instance()->getOrCreateResource<Texture>(ResourceName::Texture, Name("../resources/box.jpg"));
		SamplerDesc samplerDesc{};
		this->sampler = RenderSystem::instance()->createSampler(samplerDesc);
		
		mCam = new Camera(Name("Scene"));
		auto rsys = RenderSystem::instance();
		mCube = new CubeEntity();
		mCube->createPass(PassName::MainCameraPass);
		mCubeB = new CubeEntity();
		mCubeB->createPass(PassName::MainCameraPass);
	}
	SimpleScene::~SimpleScene() {
		delete mCube;
		delete mCubeB;
	}

	void SimpleScene::updateScene(float deltatime)
	{
		static mat4 matrix(1.0);
		static float timeTotal = 0.;
		timeTotal += deltatime;
		vec3 cubeBTranslate = vec3(0, std::sin(timeTotal), 1.);
		auto pass = mCube->getMaterial()->getMaterialPass(PassName::MainCameraPass);
		auto texturePos = RenderSystem::instance()->getBindingPos("BoxTex", pass);
		auto samplerPos = RenderSystem::instance()->getBindingPos("BoxSampler", pass);
		auto bindingPos = RenderSystem::instance()->getBindingPos("ObjectCommon", pass);
		mat4 matrixB = translate(mat4(1.0), cubeBTranslate);
		RenderSystem::instance()->setCurrentCamera(mCam);
		matrix = rotate(matrix, deltatime, vec3(1, 1, 1));

		RenderSystem::instance()->updateUniformBufferData(bindingPos, &matrix, sizeof(mat4), mCube->getPass(PassName::MainCameraPass));
		RenderSystem::instance()->updateUniform(texturePos,0, texture->getRsImage(), mCube->getPass(PassName::MainCameraPass));
		RenderSystem::instance()->updateUniform(samplerPos,0, sampler, mCube->getPass(PassName::MainCameraPass));

		RenderSystem::instance()->updateUniformBufferData(bindingPos, &matrixB, sizeof(mat4), mCubeB->getPass(PassName::MainCameraPass));
		RenderSystem::instance()->updateUniform(texturePos,0, texture->getRsImage(), mCubeB->getPass(PassName::MainCameraPass));
		RenderSystem::instance()->updateUniform(samplerPos,0, sampler, mCubeB->getPass(PassName::MainCameraPass));
		auto mainRenderQueue = RenderSystem::instance()->getMainRenderQueue();
		mainRenderQueue->submit(mCube);
		mainRenderQueue->submit(mCubeB);
	}

}