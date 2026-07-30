#include "Renderer/HiZ.h"
#include "Renderer/TextureResourceMgr.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/RenderDebuger.h"
namespace Render {
	void HiZ::setDepth(const TexturePtr& tex)
	{
		mDepth = tex;
	}

	Render::TexturePtr HiZ::getHiZPyramid()
	{
		return mHiZPyramid;
	}

	void HiZ::execute(rs_commandbuffer* cmdBuffer)
	{
		if (!mDepth)return;
		RenderMarker marker(cmdBuffer,"HiZ Pyramid",0,1,1,1);
		setupResource();

		for (int i = 0;i < mHiZPyramid->getMips();++i) {
			int curW = std::max(1u ,mDepth->getWidth()  >> (i + 1));
			int curH = std::max(1u, mDepth->getHeight() >> (i + 1));
			if (i == 0)
			{
				mHizKernel->setParameter(mParamLastZ, mDepth, ImageViewKey().setAspect(ViewAspect::Depth));
			}
			else
			{
				mHizKernel->setParameter(mParamLastZ, mHiZPyramid, ImageViewKey().setBaseMip(i - 1).setMipCount(1).setUAVAccess(UAVAccess::ReadOnly));
			}
			mHizKernel->setParameter(mParamWriteZ, mHiZPyramid, ImageViewKey().setBaseMip(i).setMipCount(1).setUAVAccess(UAVAccess::WriteOnly));
			mHizKernel->dispatch(cmdBuffer, (curW + 7) / 8, (curH + 7) / 8, 1);
		}
	}

	HiZ::HiZ()
	{
		mHizKernel = new ComputeKernel(
			"../shader/HiZ.cs", {}
		);
		mParamLastZ = Name("u_lastZ");
		mParamWriteZ = Name("u_writeZ");
	}

	HiZ::~HiZ()
	{
		delete mHizKernel;
	}

	void HiZ::setupResource()
	{
		//Mips to go

		auto curDepthWidth = mDepth->getWidth();
		auto curDepthHeight = mDepth->getHeight();
		if (getHiZPyramid()) {
			auto curWidth = this->getHiZPyramid()->getWidth();
			auto curHeight = this->getHiZPyramid()->getHeight();
			if (curWidth == (curDepthWidth >> 1) && curHeight == (curDepthHeight >> 1)) {
				return;
			}
		}
		auto minEdge = std::min(curDepthHeight, curDepthWidth);
		int mipToGo = 0;
		while ((minEdge >>= 1) > 0) {
			mipToGo++;
		}
		
		mHiZPyramid = TextureResourceManager::instance()->createRenderTexture(RenderTextureFormat::R16F,
			curDepthWidth >> 1, curDepthHeight >> 1, 1, mipToGo, 1, true
		);

	}

}


