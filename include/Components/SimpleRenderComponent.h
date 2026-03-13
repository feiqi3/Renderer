#ifndef SIMPLE_RENDER_COMPONENT_H_
#define SIMPLE_RENDER_COMPONENT_H_
#include "function/Component.h"
#include "Renderer/MaterialInstance.h"
#include "Renderer/Mesh.h"
namespace Render {
	class SimpleRenderComponent : public Component {
	public:
		virtual void onUpdate(float dt) override;
		virtual void onDestroy() override;
	private:
		RenderEntity* mRenderEntity = nullptr;
	};
}
#endif