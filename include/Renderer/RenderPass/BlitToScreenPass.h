#ifndef BLIT_TO_SCREEN_H
#define BLIT_TO_SCREEN_H

#include"Renderer/RenderPass.h"
#include <list>
#include "render_resource.h"
namespace Render {

	class BlitToScreenPass :public RenderPass {
	public:
		BlitToScreenPass();
		~BlitToScreenPass();

		void initBlitData();
		void setBlitRT(rs_image* rt);
		virtual void drawImpl(rs_commandbuffer* cmdbuffer);

	private:
		std::list<
	};

};

#endif