#include "components/RenderComponent.h"
#include "Renderer/EngineCullMasks.h"
#include "function/Object.h"
namespace Render {

	RenderComponent::RenderComponent()
		: mCullMask(CullMask::MainCamera)
		, mCastShadow(true)
		, mSceneIndex(static_cast<size_t>(-1))
	{

	}

	RenderComponent::~RenderComponent()
	{
		if (owner() && owner()->scene()) {
			owner()->scene()->registerRenderComponent(this);
		}
	}

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