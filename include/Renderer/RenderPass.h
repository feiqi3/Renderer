#ifndef RENDER_PASS_H
#define RENDER_PASS_H
#include "render_resource_createinfo.h"  
#include <string>
namespace Render {
	struct rs_renderpass;
	struct rs_rendertarget;
	struct rs_commandbuffer;
	class RenderPass {
	public:
		RenderPass(const std::string& passName, const PassDesc& desc);
		virtual ~RenderPass();
		void setRenderTarget(rs_rendertarget* renderTarget);
		void draw(rs_commandbuffer* cmdbuffer);
	protected: 
		virtual void drawImpl(rs_commandbuffer* cmdbuffer) = 0;
	protected:
		std::string mPassName;
		rs_renderpass* mRenderPass;
		PassDesc mPassDesc;
	};
}

#endif