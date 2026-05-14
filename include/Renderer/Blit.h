#ifndef BLIT_H_
#define BLIT_H_
#include "Renderer/TextureResourceMgr.h"
#include "Renderer/SamplerResourceManager.h"
#include "Renderer/RenderSystem.h"
namespace Render {
	class Blitor {
	public:
		Blitor();
		~Blitor();
	public:
		void cmdBlit(rs_commandbuffer* cmd, TexturePtr BlitFrom, ImageViewKey viewKeyFrom, TexturePtr BlitTo, ImageViewKey viewKeyTo, Filter filter = Filter::Nearest);
	private:
		SamplerPtr mBlitSamplerNearest;
		SamplerPtr mBlitSamplerBilinear;
		class ComputeKernel* mBlitKernel;
	};
}

#endif