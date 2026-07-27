#include "Renderer/Blit.h"
#include "Renderer/ComputeKernel.h"
namespace Render {
	Blitor::Blitor()
	{
		mBlitKernel = new ComputeKernel("../shader/Blit.cs", {}, "main");
		mBlitSamplerNearest = SamplerResourceManager::instance()->getOrCreateSampler({
			.addressU = AddressMode::ClampToEdge,
			.addressV = AddressMode::ClampToEdge,
			.addressW = AddressMode::ClampToEdge,
			.minFilter = Filter::Nearest,
			.magFilter = Filter::Nearest,
			.mipmapMode = Filter::Nearest
			});
		mBlitSamplerBilinear = SamplerResourceManager::instance()->getOrCreateSampler({
		.addressU = AddressMode::ClampToEdge,
		.addressV = AddressMode::ClampToEdge,
		.addressW = AddressMode::ClampToEdge,
		.minFilter = Filter::Linear,
		.magFilter = Filter::Linear,
		.mipmapMode = Filter::Nearest
				});
	}
	Blitor::~Blitor()
	{
		delete mBlitKernel;
		mBlitKernel = nullptr;
	}
	void Blitor::cmdBlit(rs_commandbuffer* cmd, TexturePtr BlitFrom, ImageViewKey viewKeyFrom, TexturePtr BlitTo, ImageViewKey viewKeyTo,Filter filter)
	{
		const static Name paramFrom = Name("blitFrom");
		const static Name paramTo = Name("blitTo");
		const static Name paramSampler = Name("blitSampler");

		if (filter == Filter::Nearest) {
			mBlitKernel->setParameter(paramSampler, mBlitSamplerNearest);
		}
		else {
			mBlitKernel->setParameter(paramSampler, mBlitSamplerBilinear);
		}

		mBlitKernel->setParameter(paramFrom, BlitFrom, viewKeyFrom);
		mBlitKernel->setParameter(paramTo, BlitTo, viewKeyTo);

		mBlitKernel->dispatch(cmd, 
			((viewKeyTo.getMipCount() > 1 ? BlitTo->getWidth() >> viewKeyTo.getBaseMip() : BlitTo->getWidth()) + 7) / 8,
			((viewKeyTo.getMipCount() > 1 ? BlitTo->getHeight() >> viewKeyTo.getBaseMip() : BlitTo->getHeight()) + 7) / 8,
			1);

	}
}