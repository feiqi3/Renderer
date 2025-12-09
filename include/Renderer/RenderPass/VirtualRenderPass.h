#ifndef VIRTUAL_RENDER_PASS_H_
#define VIRTUAL_RENDER_PASS_H_
#include "Renderer/RenderPass.h"
namespace Render {
	class VirtualRenderPass : public RenderPass {
	public:
		VirtualRenderPass();
		~VirtualRenderPass();
		void drawImpl(rs_commandbuffer*)override;
	private:
		rs_rendertarget* mVirtualRt = 0;
		struct rs_image* mVirtualRtImage = 0;
	};
}

#endif
