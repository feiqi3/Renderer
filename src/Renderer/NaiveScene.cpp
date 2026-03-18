#include "Renderer/NaiveScene.h"
#include "function/Object.h"
#include "common/CommonMath.h"
#include "Components/SimpleRenderComponent.h"
namespace Render {
	class MoveComponent : public Component {
	public:
		MoveComponent(vec3 center, float r)
			: centerOfCircle(center), radius(r)
		{
			
		}

		virtual void onUpdate(float deltaTime)override {
			auto obj = this->owner();
			float speed = 0.1;
			vec3 locationNow = centerOfCircle + vec3(0, 1, 0) * radius * std::sin(t++);
			obj->setLocalPosition(locationNow);
		};

	private:
		int t = 0;
		vec3 centerOfCircle;
		float radius;
	};
	
	Scene* Render::createSceneByCode()
	{
		Scene* naiveScene = new Scene();
		auto objectA = naiveScene->createObject("CubeObject1");
		objectA->addComponent<SimpleRenderComponent>();
		auto objectB = naiveScene->createObject("CubeObject2");
		objectB->addComponent<SimpleRenderComponent>();
		objectB->addComponent<MoveComponent>( vec3(0,0,15),3 );
		return naiveScene;
	}

}