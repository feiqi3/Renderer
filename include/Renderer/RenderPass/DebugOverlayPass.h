#ifndef DEBUG_OVERLAY_PASS_H_
#define DEBUG_OVERLAY_PASS_H_
#include "Renderer/RenderPass.h"
#include "Renderer/TextureResourceMgr.h"
namespace Render {
	struct rs_rendertarget;
	class DebugOverlayPass : public RenderPass{
	public:
		DebugOverlayPass();
		~DebugOverlayPass();
		void init();
		void deinit();
		virtual void preDraw(rs_commandbuffer* cmdbuffer, Camera* cam)override;
		virtual void draw(rs_commandbuffer* cmdbuffer, Camera* cam)override;
		virtual void setGameView(TexturePtr image);
		void addDrawEntity(RenderEntity* entity);
		TexturePtr getOverlayTexture();
	private:
		std::vector<RenderEntity*> mEntities;
		TexturePtr mRenderTexture = nullptr;
		rs_rendertarget* mRenderTarget = nullptr;
		TexturePtr mGameView = nullptr;
		int lastFrameWinX = 0;
		int lastFrameWinY = 0;
	};
}

#endif //DEBUG_OVERLAY_PASS_H_