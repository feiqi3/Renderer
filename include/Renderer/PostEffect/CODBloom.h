#ifndef COD_BLOOM_H_
#define COD_BLOOM_H_
#include "Renderer/Texture.h"
#include "Renderer/SamplerResourceManager.h"
namespace Render {
	class CodBloomPrivate;
	class ComputeKernel;
	class CodBloom {
	public:
		CodBloom();
		~CodBloom();
		void		draw(rs_commandbuffer* cmd,const TexturePtr& mainRT);
		TexturePtr	outBloomTex()const;
		void		setBloomThreshold(float f);
		void		setBloomStrength(float f);
		void		setBloomRadius(float f);
	private:
		void		updateCfg();
		void		prepareMipmapChain();
	private:
		void		setUseKaris(bool b);
	private:
		std::unique_ptr<CodBloomPrivate> mDp;

	};

}

#endif