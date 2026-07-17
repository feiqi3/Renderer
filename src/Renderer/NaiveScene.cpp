#include "Renderer/NaiveScene.h"
#include "function/Object.h"
#include "common/CommonMath.h"
#include "Renderer/GltfLoader.h"

#include "Components/LightComponent.h"
#include "Components/FPSControllerComponent.h"
#include "Components/SkyboxRenderComponent.h"
#include "Renderer/CameraManager.h"
#include "Components/PBRRenderComponent.h"
#include "Renderer/MaterialTemplateManager.h"
#include "Renderer/MaterialManager.h"
#include "Renderer/Materials/PBRMaterial.h"

#include "Renderer/Camera.h"
#include "Renderer/TextureResourceMgr.h"
#include "Renderer/SamplerResourceManager.h"
#include "common/ResourceSystem.h"
#include "Renderer/MeshResourceManager.h"
#include "Renderer/Mesh.h"
#include "Renderer/RenderQueue.h"
#include "Renderer/GPUShared/PBREntity.h"
#include "Renderer/EnginePass.h"

#include "Components/RenderDebugUIComponent.h"
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
		auto model = loader.createFromFilePath("../resources/Sponza/Sponza.gltf");
		auto node = loader.toEngineSceneNode(naiveScene, model);
		node->setLocalScale(vec3(3, 3, 3));
		node->setLocalPosition(vec3(0,-10, -40));
		node->setLocalRotation(fromAxisAngle(vec3(0, 1, 0), 90));
		//node->addComponent<SpinComponent>();
		auto nodeLight = naiveScene->createObject("Lights");
		auto pointLightcomponent = nodeLight->addComponent<PointLightComponent>();
		nodeLight->setLocalPosition(vec3(0, 10, -35));
		nodeLight->addComponent<MoveComponent>();
		pointLightcomponent->setRange(150);
		pointLightcomponent->setIntensity(50.f);
		pointLightcomponent->setColor(vec3(1.));
		delete model;
		auto debugNode = naiveScene->createObject("DebugObject");
		debugNode->addComponent<RenderDebugUIComponent>();
		auto cameraNode = naiveScene->createObject("Camera");
		auto cameraComponent = cameraNode->addComponent<FPSControllerComponent>();
		auto camera = CameraManager::instance()->getCamera(Name("SceneMainCamera"));
		camera->setFar(1500.f);
		cameraComponent->setCamera(camera);

		auto directionalLightcomponent = nodeLight->addComponent<DirectionalLightComponent>();
		directionalLightcomponent->setDirection(vec3(-0.1f, -1.0f, 0.1f));
		directionalLightcomponent->setColor(vec3(1.0, 0.95, 0.9));
		directionalLightcomponent->setIntensity(15.f);
		directionalLightcomponent->setHasShadow(true);
		auto skyBoxNode = naiveScene->createObject("Skybox");
		auto skyBoxComponent = skyBoxNode->addComponent<SkyboxRenderComponent>();
		skyBoxComponent->setSkybox(TextureResourceManager::instance()->getOrCreateCubemap(Name("../resources/skyboxes/CityViewHdr")));
		skyBoxComponent->setSampler(
			ResourceSystem::instance()->getDefaultResource<Sampler>(Sampler::typeName())
		);

		auto cubeMesh = ResourceSystem::instance()->getResource<Mesh>(Mesh::typeName(), Name("Builtin::Cube"));
		auto cubeNode = naiveScene->createObject("Cube");
		cubeNode->setLocalPosition(vec3(10, 10, 10));
		cubeNode->setLocalScale(vec3(3, 3, 3));
		auto cubeRenderer = cubeNode->addComponent<PBRRenderComponent>();
		cubeRenderer->setMesh(cubeMesh);
		auto pbrTemplate = MaterialTemplateManager::instance()->getMaterialTemplate(Name("PBRMaterialTemplate_Opaque"));
		auto cubeMaterial = MaterialManager::instance()->createMaterial<PBRMaterial>(Name("CubeMaterial"), pbrTemplate);
		cubeRenderer->setMaterial(0, cubeMaterial);
		cubeMaterial->setRenderOrder(RenderOrder::Opaque);
		auto cubePBRMaterial = (PBRMaterial*)cubeMaterial.get();
		cubePBRMaterial->setMetallic(0.5);
		cubePBRMaterial->setRoughness(0.25);
		
		cubePBRMaterial->setBaseColor(vec4(0.5,0.5,0.5,1.));

		Scene::setCurrentScene(naiveScene);
		return naiveScene;
	}

}