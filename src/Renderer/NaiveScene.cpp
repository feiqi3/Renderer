#include "Renderer/NaiveScene.h"
#include "function/Object.h"
#include "common/CommonMath.h"
#include "Components/SimpleRenderComponent.h"
#include "Renderer/GltfLoader.h"
#include "Components/LightComponent.h"
namespace Render {
	class MoveComponent : public Component {
	public:
		MoveComponent()
		{
			
		}

		virtual void onUpdate(float deltaTime)override {
			t += deltaTime * 0.25f;
			float x = sin(t) * 30;
			auto loc = this->owner()->localPosition();
			this->owner()->setLocalPosition(vec3(x, loc.y, loc.z));
		};

	private:
		float t = 0;
	};
	
	class SpinComponent : public Component {
	public:
		SpinComponent()
		{

		}

		virtual void onUpdate(float deltaTime)override {
			auto obj = this->owner();
			float speed = 0.05;
			xDeg += speed * deltaTime;
			owner()->setLocalRotation(fromAxisAngle(vec3(0,1,0), xDeg));
		};

	private:
		float xDeg = 0;
		int t = 0;
	};


	Scene* Render::createSceneByCode()
	{
		Scene* naiveScene = new Scene();
		//auto objectA = naiveScene->createObject("CubeObject1");
		//objectA->addComponent<SimpleRenderComponent>();
		//objectA->setLocalPosition(vec3(0, 0, -2));
		//auto objectB = naiveScene->createObject("CubeObject2");
		//objectB->addComponent<SimpleRenderComponent>();
		//objectB->addComponent<MoveComponent>( vec3(0,0,-5),3 );
		GLTFLoader loader;
		auto model = loader.createFromFilePath("../resources/Fox/fox.gltf");
		auto node = loader.toEngineSceneNode(naiveScene, model);
		node->setLocalScale(vec3(0.25, 0.25, 0.25));
		node->setLocalPosition(vec3(0,-10, -40));
		node->setLocalRotation(fromAxisAngle(vec3(0, 1, 0), 90));
		//node->addComponent<SpinComponent>();
		auto nodeLight = naiveScene->createObject("DirLightNode");
		auto pointLightcomponent = nodeLight->addComponent<PointLightComponent>();
		nodeLight->setLocalPosition(vec3(0, 10, -35));
		nodeLight->addComponent<MoveComponent>();
		pointLightcomponent->setRange(150);
		pointLightcomponent->setIntensity(50.f);
		pointLightcomponent->setColor(vec3(1.));
		delete model;
		Scene::setCurrentScene(naiveScene);
		return naiveScene;
	}

}