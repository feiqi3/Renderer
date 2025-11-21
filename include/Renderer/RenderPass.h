#ifndef RENDER_PASS_H
#define RENDER_PASS_H
#include "common/Name.h"
#include "render_resource_createinfo.h"  
#include <string>
namespace Render {
	struct rs_renderpass;
	struct rs_rendertarget;
	struct rs_commandbuffer;
	class RenderPass {
	public:
		RenderPass(const Name& passName, const PassDesc& desc);
		virtual void init() {}
		virtual ~RenderPass();
		virtual void updateViewportAndScissor(rs_commandbuffer* cmdbuffer);
		rs_renderpass* getRaw()const { return mRenderPass; }
		void setRenderTarget(rs_rendertarget* renderTarget,bool compatible = false);
		void draw(rs_commandbuffer* cmdbuffer);
		void setClearData(const std::vector<ClearColor>& clrColor, ClearDepthStencil dsClear) {
			mClrColor = clrColor;
			mDsClear = dsClear;
		}

		virtual void beginFrame(uint64_t frame) {};
		const Name& getPassName() { return mPassName; }
	protected: 
		friend class RenderPassManager;
		virtual void drawImpl(rs_commandbuffer* cmdbuffer) = 0;
	protected:
		Name mPassName;
		rs_renderpass* mRenderPass;
		const PassDesc mPassDesc;
		std::vector<ClearColor> mClrColor;
		ClearDepthStencil mDsClear;
	};
}

#endif