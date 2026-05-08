#include "Renderer/PostEffect/CODBloom.h"
#include "Renderer/ComputeKernel.h"
#include "Renderer/Texture.h"
#include "Renderer/TextureResourceMgr.h"
namespace Render {
	//Next-Generation-Post-Processing-in-Call-of-Duty-Advanced-Warfare

	struct CODBloomCFG {
		float BloomStrength;
		float Threshold;					//TODO
		float Radius = 0.15;                //Up sample parameter, controls the uv step. 
		float Padding3;
	};

	CodBloom::CodBloom()
	{
		mDownsampleKernel = new ComputeKernel("../shader/PostEffect/CodBloom.cs",
			{"DOWN_SAMPLE",""},
			"DownSampleMain"
		);
		mUpsampleKernel = new ComputeKernel("../shader/PostEffect/CodBloom.cs", 
			{ "UP_SAMPLE","" }, 
			"UpSampleMain"
		);
	}

	CodBloom::~CodBloom()
	{
		delete mDownsampleKernel;
		delete mUpsampleKernel;

	}

	void CodBloom::draw(TexturePtr mainRT)
	{
		if (mSavedMainRT != mainRT) {
			mSavedMainRT = mainRT;
			prepareMipmapChain();
		}
		//Build Mip chain downsample
		//TODO

		//Build Mip chain upsample
		//TODO

		//Compose pass....  
	}

	void CodBloom::prepareMipmapChain()
	{
		auto& mainRT = mSavedMainRT;
		uint32_t mip0x = mainRT->getWidth() >> 1;
		uint32_t mip0y = mainRT->getHeight() >> 1;
		auto minEdge = std::min(mip0x, mip0y);
		int mip = 1;
		while (minEdge /= 2) {
			if (minEdge < 20)break;
			mip++;
		}
		mip = std::max(mip, 6);
		mTempMipChain = TextureResourceManager::instance()->createRenderTexture(RenderTextureFormat::R11G11B10F, mip0x, mip0y,1, mip, 1);
		
	}

}