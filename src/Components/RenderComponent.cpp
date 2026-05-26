#include "components/RenderComponent.h"
namespace Render {

	void RenderComponent::onOwnerSetScene(Scene* originScene, Scene* scene)
	{
		if (originScene) {
			originScene->unregisterRenderComponent(this);
		}
		if (scene) {
			scene->registerRenderComponent(this);
		}
	}

}