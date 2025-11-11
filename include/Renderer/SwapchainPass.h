#ifndef SWAP_CHAIN_PASS_H
#define SWAP_CHAIN_PASS_H

#include"RenderPass.h"
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
		class RenderEntity* BlitEntity = 0;
		class MaterialTemplate* BlitMaterial = 0;
		class Material* BlitMatVarient = 0;
		class rs_sampler* BlitRTSampler = 0;
		class Pass* BlitPass = 0;
		rs_drawdata* BlitDrawData = 0;
		rs_binding_pos BlitTarget = INVALID_BINDING_POS;
		rs_binding_pos BlitSampler = INVALID_BINDING_POS;
	};

};

#endif