#include "Renderer/RenderPass.h"
#include "Vulkan/vulkan_render_function.h"
#include "Vulkan/vulkan_pipeline.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/RenderDebuger.h"
#include "Renderer/RenderPassManager.h"
#include "Renderer/MaterialTemplateManager.h"
namespace Render {
	RenderPass::RenderPass(const Name& passName, const PassDesc& desc) :mPassName(passName), mRenderPass(nullptr), mPassDesc(desc)
	{
		auto ctx = RenderSystem::instance()->getRenderContext();
		mRenderPass = createRsRenderPassVk(ctx, mPassDesc);
	}
	void RenderPass::setRenderTarget(rs_rendertarget* renderTarget)
	{
		using namespace Vulkan;
		if (renderTarget == nullptr) {
			mRendertarget = nullptr;
			return;
		}

		auto ctx = RenderSystem::instance()->getRenderContext();
		if (!RenderSystem::instance()->isRenderTargetCompatibleToRenderPass(mRenderPass, renderTarget)) {
			assert(0);
			return;
		}
		this->mRendertarget = renderTarget;
	}
	void RenderPass::draw(rs_commandbuffer* cmdbuffer)
	{
		RenderMarker Marker(cmdbuffer, mPassName.c_str(), 1.f, 0.f, 0.f, 1.f);
		RenderSystem::instance()->cmdBeginRenderPass(cmdbuffer, mRenderPass, mClrColor, mDsClear);
		if (mRendertarget) {
			RenderSystem::instance()->cmdSetRendertarget(cmdbuffer, mRendertarget);
			updateViewportAndScissor(cmdbuffer, mRendertarget);
		}
		drawImpl(cmdbuffer);
		RenderSystem::instance()->cmdEndRenderPass(cmdbuffer);
	}

	bool RenderPass::needRebuildPipeline(rs_rendertarget* oldrt, rs_rendertarget* newrt)
	{
		auto oldrtVk = (Vulkan::rs_rendertarget_vk*)oldrt;
		auto newrtVk = (Vulkan::rs_rendertarget_vk*)newrt;

		auto imageCompatibleTest = [](const rs_image& a, const rs_image& b) {
			if (a.format != b.format) {
				return true;
			}

			if (a.sampleCount != b.sampleCount) {
				return true;
			}

			if (a.usage != b.usage) {
				return true;
			}
			return false;
		};

		if (oldrt->m_attachments.size() != newrt->m_attachments.size() || ((oldrt->m_depthStencilAttachment == nullptr) ^ (newrt->m_depthStencilAttachment != nullptr))){		return true;
			return true;
		}
		for (int i = 0;i < oldrt->m_attachments.size();++i) {
			auto attOld = oldrt->m_attachments[i];
			auto attNew = newrt->m_attachments[i];
			if (imageCompatibleTest(*attOld,*attNew)) {
				return true;
			}

		}

		if (
			oldrt->m_depthStencilAttachment && newrt->m_depthStencilAttachment &&
			imageCompatibleTest(*oldrt->m_depthStencilAttachment, *newrt->m_depthStencilAttachment)) {
			return true;
		}

		return false;
	}

	RenderPass::~RenderPass()
	{
		using namespace Vulkan;
		auto ctx = RenderSystem::instance()->getRenderContext();
		auto pass = (rs_renderpass_vk*)mRenderPass;
		destroyRsRenderPassVk(ctx, pass);
		mRenderPass = 0;
	}
	void RenderPass::updateViewportAndScissor(rs_commandbuffer* cmdbuffer,rs_rendertarget* rt)
	{
		Rect2D rect{};
		rect.l = 0.f;
		rect.t = 0.f;
		rect.b = 1.f;
		rect.r = 1.f;

		if (rt) {
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