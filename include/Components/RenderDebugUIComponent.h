#ifndef RENDER_DEBUG_UI_H_
#define RENDER_DEBUG_UI_H_
#include "Renderer/Texture.h"
#include "function/Component.h"
namespace Render {
	class RenderDebugUIComponent : public Component {
	public:
		enum class DebugView {
			Depth,
			DirShadow,
			DirShadow_CSM_1,
			DirShadow_CSM_2,
			DirShadow_CSM_3,
			Max
		};
		inline static const char* DebugViewName[] = {
			"Depth",
			"DirShadow",
			"DirShadow_CSM_1",
			"DirShadow_CSM_2",
			"DirShadow_CSM_3",
			"None"
		};
		virtual void onUpdate(float dt)override;
	private:
		void writePassTable();
		rs_image_view* getDebugTextureView(DebugView view);
	private:
		TexturePtr mainDepth;
	};
}

#endif //!RENDER_DEBUG_UI_H_