#ifndef SWAP_CHAIN_PASS_H
#define SWAP_CHAIN_PASS_H

#include"RenderPass.h"
#include "Renderer/MaterialTemplate.h"
#include "render_resource.h"
namespace Render {

	class SwapchainPass :public RenderPass{
	public:
		SwapchainPass();
		void init()override;
		~SwapchainPass();
		
		void initBlitData();
		void setBlitRT(rs_image* rt);
		virtual void drawImpl(rs_commandbuffer* cmdbuffer);

	private:
		class NormalEntity* BlitEntity = 0;
		struct rs_sampler* BlitRTSampler = 0;
		class Pass* BlitPass = 0;
		rs_drawdata* BlitDrawData = 0;
		rs_binding_pos BlitTarget = INVALID_BINDING_POS;
		rs_binding_pos BlitSampler = INVALID_BINDING_POS;
	};

};

#endif