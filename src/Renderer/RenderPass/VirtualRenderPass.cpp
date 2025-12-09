#include "Renderer/RenderPass/VirtualRenderPass.h"
#include "Renderer/RenderSystem.h"
#include <stdexcept>
namespace Render
{
	static PassDesc getRenderPassDesc() {
		PassDesc ret;
		ret.attachments.push_back(PassAttachment{});
		return ret;
	}

	VirtualRenderPass::VirtualRenderPass():RenderPass(Name("VirtualRenderPass"),getRenderPassDesc())
	{
		this->mVirtualRtImage = RenderSystem::instance()->createRTTexture(ImageFormat::BGRA8_UNORM,10,10,1,1,false);
		this->mVirtualRt = RenderSystem::instance()->createRendertarget({ mVirtualRtImage }, 0);
		this->setRenderTarget(mVirtualRt);
	}

	VirtualRenderPass::~VirtualRenderPass()
	{
		this->setRenderTarget(nullptr);
		auto rsys = RenderSystem::instance();
		rsys->destroyImage(mVirtualRtImage);
		mVirtualRtImage = 0;
		rsys->destroyRenderTarget(mVirtualRt);
		mVirtualRt = 0;
	}

	void VirtualRenderPass::drawImpl(rs_commandbuffer*)
	{
		assert(0);
		throw std::runtime_error("Should not be here!");
	}

}
