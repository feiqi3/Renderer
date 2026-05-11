#include "Renderer/RenderPass.h"
#include "Vulkan/vulkan_render_function.h"
#include "Vulkan/vulkan_pipeline.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/RenderDebuger.h"
#include "Renderer/RenderPassManager.h"
#include "Renderer/MaterialTemplateManager.h"
#include "Renderer/RenderQueue.h"
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
		collectRenderEntities(mRenderPacks);
		//Update parameters must before renderpass begin.
		for (auto&& pack : mRenderPacks) {
			RenderSystem::instance()->updateParameters(cmdbuffer, pack.entity, pack.pass);
		}
		RenderSystem::instance()->excutePendingBufferCopies(cmdbuffer);
		RenderSystem::instance()->cmdBeginRenderPass(cmdbuffer, mRenderPass, mClrColor, mDsClear);
		if (mRendertarget) {
			RenderSystem::instance()->cmdSetRendertarget(cmdbuffer, mRendertarget);
			updateViewportAndScissor(cmdbuffer, mRendertarget);
		}
		drawImpl(cmdbuffer);
		RenderSystem::instance()->cmdEndRenderPass(cmdbuffer);
		mRenderPacks.clear();
	}

	void RenderPass::collectRenderEntities(std::vector<RenderPack>& packs)
	{
		auto view = RenderSystem::instance()->getMainRenderQueue()->getView(this->getPassName());
		while (true) {
			auto renderData = view.next();
			if (renderData == nullptr)break;
			RenderPack pack{ .entity = renderData->entity,
				.pass = renderData->entity->getPass(this->getPassName())
			};
			packs.push_back(pack);
		}
	}

	void RenderPass::drawImpl(rs_commandbuffer* cmdbuffer)
	{
		for (auto& pack : mRenderPacks) {
			RenderSystem::instance()->drawIndexed(cmdbuffer, pack.entity, pack.pass);
		}
	}

	bool RenderPass::needRebuildPipeline(rs_rendertarget* oldrt, rs_rendertarget* newrt)
	{
		return !isRTCompatible(oldrt, newrt);
	}

	bool RenderPass::isRTCompatible(rs_rendertarget* rtA, rs_rendertarget* rtB)
	{

		auto imageCompatibleTest = [](const rs_image_view* va, const rs_image_view* vb) {
			if (!va || !vb || !va->image || !vb->image) {
				return false;
			}

			auto& aViewKey = va->viewKey;
			auto& bViewKey = vb->viewKey;

			uint32_t mipA = aViewKey.getBaseMip();
			uint32_t mipB = bViewKey.getBaseMip();
			uint32_t mipCountA = aViewKey.getMipCount(); 
			uint32_t mipCountB = bViewKey.getMipCount();

			auto a = va->image;
			auto b = vb->image;  

			if (a->format != b->format) return false;
			if (a->sampleCount != b->sampleCount) return false;
			if (a->usage != b->usage) return false;
			if (a->type != b->type) return false; 

			if (mipCountA != mipCountB) return false;
			if (aViewKey.getLayerCount() != bViewKey.getLayerCount()) return false;

			auto getMipSize = [](uint16_t size, uint32_t mip) -> uint32_t {
				return std::max<uint32_t>(1, static_cast<uint32_t>(size) >> mip);
				};

			uint32_t widthA = getMipSize(a->width, mipA);
			uint32_t heightA = getMipSize(a->height, mipA);
			uint32_t depthA = getMipSize(a->depth, mipA);

			uint32_t widthB = getMipSize(b->width, mipB);
			uint32_t heightB = getMipSize(b->height, mipB);
			uint32_t depthB = getMipSize(b->depth, mipB);

			if (widthA != widthB || heightA != heightB || depthA != depthB) {
				return false;
			}

			return true;
			};

		if (rtA->m_attachments.size() != rtB->m_attachments.size() ||
			(
				(rtA->m_depthStencilAttachment == nullptr) ^
				(rtB->m_depthStencilAttachment != nullptr)
			)
		   ) {
			return false;
		}
		for (int i = 0;i < rtA->m_attachments.size();++i) {
			auto attOld = rtA->m_views[i];
			auto attNew = rtB->m_views[i];
			if (!imageCompatibleTest(attOld, attNew)) {
				return false;
			}

		}

		return true;

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