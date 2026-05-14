#include "Renderer/PostEffect/CODBloom.h"
#include "Renderer/ComputeKernel.h"
#include "Renderer/Texture.h"
#include "Renderer/TextureResourceMgr.h"
#include "Renderer/SamplerResourceManager.h"

namespace Render {
	//Next-Generation-Post-Processing-in-Call-of-Duty-Advanced-Warfare

	struct CODBloomCFG {
		float BloomStrength;
		float Threshold;
		float Radius;               //Up sample parameter, controls the uv step. 
		float Karis;                //Do Karis? alway in downsample mip0 -> mip1
	};

	class CodBloomPrivate {
	public:
		ComputeKernel* mDownsampleKernel;
		ComputeKernel* mUpsampleKernel;
		TexturePtr	   mTempMipChain;
		TexturePtr	   mSavedMainRT;
		SamplerPtr	   mMipChainSampler;
		CODBloomCFG	   mBloomCFG = {};
		rs_buffer*	   mCfgBuffer = nullptr;
		bool		   mCfgDirty =	true;
	};

	CodBloom::CodBloom()
	{
		mDp = std::make_unique<CodBloomPrivate>();
		mDp->mDownsampleKernel = new ComputeKernel("../shader/PostEffect/CodBloom.cs",
			{ {"DOWN_SAMPLE",""}  },
			"DownSampleMain"
		);
		mDp->mUpsampleKernel = new ComputeKernel("../shader/PostEffect/CodBloom.cs",
			{ {"UP_SAMPLE",""} }, 
			"UpSampleMain"
		);
		SamplerDesc sampDesc{};
		sampDesc.addressU = AddressMode::ClampToEdge;
		sampDesc.addressV = AddressMode::ClampToEdge;
		sampDesc.minFilter = Filter::Linear;
		sampDesc.magFilter = Filter::Linear;
		sampDesc.mipmapMode = Filter::Nearest;
		mDp->mMipChainSampler = SamplerResourceManager::instance()->getOrCreateSampler(sampDesc);
		//Bind sampler
		mDp->mDownsampleKernel->setParameter("BilinearSampler", mDp->mMipChainSampler);
		mDp->mUpsampleKernel->setParameter("BilinearSampler", mDp->mMipChainSampler);
	}

	CodBloom::~CodBloom()
	{
		delete mDp->mDownsampleKernel;
		delete mDp->mUpsampleKernel;
		mDp = nullptr;
	}

	void CodBloom::draw(rs_commandbuffer* cmd, const TexturePtr& mainRT)
	{
		if (mDp->mSavedMainRT != mainRT) {
			mDp->mSavedMainRT = mainRT;
			prepareMipmapChain();
		}
		//Build Mip chain downsample
		

		const static Name& paramMipN1Name = Name("MipN_1");
		const static Name& paramMipNName = Name("MipN");

		ImageViewKey Mip0ViewKey;
		RenderSystem::instance()->cmdBlit(cmd, mainRT, Mip0ViewKey, mDp->mTempMipChain, Mip0ViewKey, Filter::Nearest);

		//Down sample
		for (int i = 1;i < this->mDp->mTempMipChain->getMips();++i) {
			ImageViewKey MipN1ViewKey;
			MipN1ViewKey.setAspect(ViewAspect::Color).setBaseLayer(0).setBaseMip(i - 1).
				setViewType(ImageType::V2D).setUAVAccess(UAVAccess::ReadOnly);

			if (i == 1) {
				setUseKaris(true);
				//Use main rt as mip0
				mDp->mDownsampleKernel->setParameter(paramMipN1Name, mDp->mSavedMainRT, MipN1ViewKey);
			}
			else {
				setUseKaris(false);
				mDp->mDownsampleKernel->setParameter(paramMipN1Name, mDp->mTempMipChain, MipN1ViewKey);
			}
			updateCfg();

			ImageViewKey MipNViewKey;
			MipNViewKey.setAspect(ViewAspect::Color).setBaseLayer(0).
				setBaseMip(i).setViewType(ImageType::V2D).setUAVAccess(UAVAccess::ReadWrite);
			mDp->mDownsampleKernel->setParameter(paramMipNName, mDp->mTempMipChain, MipNViewKey);

			//Write to mip n
			ivec2 writeMipSize = vec2(mDp->mSavedMainRT->getWidth() >> i, mDp->mSavedMainRT->getHeight() >> i);

			mDp->mDownsampleKernel->dispatch(
				cmd,
				(writeMipSize.x + 7 ) / 8,
				(writeMipSize.y + 7 ) / 8,
				1);
		}
		setUseKaris(false);
		//Up sample
		for (int i = this->mDp->mTempMipChain->getMips() - 1;i >= 1; --i) {
			updateCfg();

			//Write to mip n - 1
			ImageViewKey MipN_1ViewKey;
			MipN_1ViewKey.setAspect(ViewAspect::Color).setBaseLayer(0).setBaseMip(i-1)
				.setViewType(ImageType::V2D).setUAVAccess(UAVAccess::ReadWrite);

			mDp->mUpsampleKernel->setParameter(paramMipN1Name, mDp->mTempMipChain, MipN_1ViewKey);

			ImageViewKey MipNViewKey;
			MipNViewKey.setAspect(ViewAspect::Color).setBaseLayer(0).setBaseMip(i)
				.setViewType(ImageType::V2D).setUAVAccess(UAVAccess::ReadOnly);
			mDp->mUpsampleKernel->setParameter(paramMipNName, mDp->mTempMipChain, MipNViewKey);
			ivec2 writeMipSize = vec2(mDp->mSavedMainRT->getWidth() >> (i - 1), mDp->mSavedMainRT->getHeight() >> (i - 1));

			mDp->mUpsampleKernel->dispatch(
				cmd,
				(writeMipSize.x + 7) / 8,
				(writeMipSize.y + 7) / 8,
				1
			);
		}
	}

	Render::TexturePtr CodBloom::outBloomTex() const
	{
		return mDp->mTempMipChain;
	}

	void CodBloom::setBloomThreshold(float f)
	{
		mDp->mBloomCFG.Threshold = f;
		mDp->mCfgDirty = true;
	}

	void CodBloom::setBloomStrength(float f)
	{
		mDp->mBloomCFG.BloomStrength = f;
		mDp->mCfgDirty = true;
	}

	void CodBloom::setBloomRadius(float f)
	{
		mDp->mBloomCFG.Radius = f;
		mDp->mCfgDirty = true;
	}

	void CodBloom::updateCfg()
	{
		if (!mDp->mCfgDirty)return;
		if (!mDp->mCfgBuffer) {
			BufferDesc bDesc{};
			bDesc.bufUsage = BufferType_Uniform;
			bDesc.byteSize = sizeof(CODBloomCFG);
			mDp->mCfgBuffer = RenderSystem::instance()->createBuffer(&mDp->mBloomCFG, sizeof(CODBloomCFG), bDesc);
		}
		else {
			RenderSystem::instance()->updateBufferData(mDp->mCfgBuffer, &(mDp->mBloomCFG), sizeof(CODBloomCFG), 0);
		}
		mDp->mCfgDirty = false;
	}

	void CodBloom::prepareMipmapChain()
	{
		auto& mainRT = mDp->mSavedMainRT;
		uint32_t mip0x = mainRT->getWidth();
		uint32_t mip0y = mainRT->getHeight();
		auto minEdge = std::min(mip0x, mip0y);
		int mip = 0;
		while (minEdge /= 2) {
			if (minEdge < 20)break;
			mip++;
		}
		mip = clamp(mip,1 , 6);
		mDp->mTempMipChain = TextureResourceManager::instance()->createRenderTexture(RenderTextureFormat::R11G11B10F, mip0x, mip0y,1, mip, 1,true);
		
	}

	void CodBloom::setUseKaris(bool b)
	{
		if (mDp->mBloomCFG.Karis == b)return;
		mDp->mBloomCFG.Karis = b;
		mDp->mCfgDirty = true;
	}

}