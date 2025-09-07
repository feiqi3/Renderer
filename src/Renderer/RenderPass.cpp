#include "Renderer/RenderPass.h"
#include "Vulkan/vulkan_render_function.h"
#include "Vulkan/vulkan_pipeline.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/RenderDebuger.h"
namespace Render {
	RenderPass::RenderPass(const std::string& passName, const PassDesc& desc) :mPassName(passName), mRenderPass(nullptr), mPassDesc(desc)
	{
	}
	void RenderPass::setRenderTarget(rs_rendertarget* renderTarget)
	{
		using namespace Vulkan;
		auto ctx = RenderSystem::instance()->getRenderContext();
		if (!this->mRenderPass) {
			mRenderPass = createRsRenderPassVk(ctx, (rs_rendertarget_vk*)renderTarget, mPassDesc);
		}
		else {
			changeRsRenderPassRtVk(ctx, (rs_renderpass_vk*)mRenderPass, (rs_rendertarget_vk*)renderTarget);
		}
	}
	void RenderPass::draw(rs_commandbuffer* cmdbuffer)
	{
		RenderMarker Marker(cmdbuffer, mPassName, 1.f, 0.f, 0.f, 1.f);
		drawImpl(cmdbuffer);
	}
	RenderPass::~RenderPass()
	{
		using namespace Vulkan;
		auto ctx = RenderSystem::instance()->getRenderContext();
		auto pass = (rs_renderpass_vk*)mRenderPass;
		destroyRsRenderPassVk(ctx, pass);
		mRenderPass = 0;
	}
};