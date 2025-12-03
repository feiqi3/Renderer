#include "Renderer/RenderPass.h"
#include "Vulkan/vulkan_render_function.h"
#include "Vulkan/vulkan_pipeline.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/RenderDebuger.h"
#include "Renderer/RenderPassManager.h"
namespace Render {
	RenderPass::RenderPass(const Name& passName, const PassDesc& desc) :mPassName(passName), mRenderPass(nullptr), mPassDesc(desc)
	{
		auto mgr = RenderSystem::instance()->getRenderPassManager();
		mgr->addRenderPass(passName, this);
		ClearColor SwapchainImgClrColor = {};
		SwapchainImgClrColor.rgba[0] = 0.f;
		SwapchainImgClrColor.rgba[1] = 0.f;
		SwapchainImgClrColor.rgba[2] = 0.f;
		SwapchainImgClrColor.rgba[3] = 1.f;
		mClrColor.push_back(SwapchainImgClrColor);
	}
	void RenderPass::setRenderTarget(rs_rendertarget* renderTarget, bool compatible)
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
		RenderMarker Marker(cmdbuffer, mPassName.c_str(), 1.f, 0.f, 0.f, 1.f);
		RenderSystem::instance()->cmdBeginRenderPass(cmdbuffer, mRenderPass, mClrColor, mDsClear);
		updateViewportAndScissor(cmdbuffer);
		drawImpl(cmdbuffer);
		RenderSystem::instance()->cmdEndRenderPass(cmdbuffer);
	}
	RenderPass::~RenderPass()
	{
		using namespace Vulkan;
		auto mgr = RenderSystem::instance()->getRenderPassManager();
		mgr->removeRenderPass(this->mPassName);
		auto ctx = RenderSystem::instance()->getRenderContext();
		auto pass = (rs_renderpass_vk*)mRenderPass;
		destroyRsRenderPassVk(ctx, pass);
		mRenderPass = 0;
	}
	void RenderPass::updateViewportAndScissor(rs_commandbuffer* cmdbuffer)
	{
		Rect2D rect{};
		rect.l = 0.f;
		rect.t = 0.f;
		rect.b = 1.f;
		rect.r = 1.f;

		if (this->mRenderPass) {
			auto rt = this->mRenderPass->renderTarget;
			auto renderSys = RenderSystem::instance();
			for (auto i = 0; i < rt->m_attachments.size(); i++) {
				renderSys->cmdSetViewport(cmdbuffer, i, 0.0f, 1.f, rect);
				renderSys->cmdSetScissor(cmdbuffer, i, rect);
			}
			if(rt->m_depthStencilAttachment){
				renderSys->cmdSetViewport(cmdbuffer, rt->m_attachments.size(), 0.0f, 1.f, rect);
				renderSys->cmdSetScissor(cmdbuffer, rt->m_attachments.size(), rect);
			}
		}
	}
};