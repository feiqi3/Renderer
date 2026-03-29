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
		virtual void updateViewportAndScissor(rs_commandbuffer* cmdbuffer, rs_rendertarget* rt);
		rs_renderpass* getRaw()const { return mRenderPass; }
		void setRenderTarget(rs_rendertarget* renderTarget);
		virtual void draw(rs_commandbuffer* cmdbuffer);
		inline void setClearData(const std::vector<ClearColor>& clrColor, ClearDepthStencil dsClear) {
			mClrColor = clrColor;
			mDsClear = dsClear;
		}

		struct RenderPack {
			class RenderEntity* entity;
			class Pass*			pass;
		};
		virtual void collectRenderEntities(std::vector<RenderPack>& pack);
		virtual void beginFrame(uint64_t frame) {};
		const Name& getPassName() { return mPassName; }
	protected:
		std::vector<RenderPack> mRenderPacks;
	protected: 
		friend class RenderPassManager;
		virtual void drawImpl(rs_commandbuffer* cmdbuffer);
		bool needRebuildPipeline(rs_rendertarget* oldrt,rs_rendertarget* newrt);
	protected:
		Name mPassName;
		rs_rendertarget* mRendertarget = nullptr;
		rs_renderpass* mRenderPass;
		const PassDesc mPassDesc;
		std::vector<ClearColor> mClrColor;
		ClearDepthStencil mDsClear;
	};
}

#endif