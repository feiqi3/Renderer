#ifndef COD_BLOOM_H_
#define COD_BLOOM_H_
#include "Renderer/Texture.h"
#include "Renderer/SamplerResourceManager.h"
namespace Render {
	class ComputeKernel;
	class CodBloom {
	public:
		CodBloom();
		~CodBloom();
		void draw(TexturePtr mainRT);
	

	private:
		void prepareMipmapChain();
	private:
		ComputeKernel* mDownsampleKernel;
		ComputeKernel* mUpsampleKernel;
		TexturePtr	   mTempMipChain;
		TexturePtr	   mSavedMainRT;
		SamplerPtr	   mDownsampleSampler;
		SamplerPtr	   mUpsampleSampler;
	};

}

#endif