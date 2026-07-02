#include "Renderer/RenderPass.h"
#include "Vulkan/vulkan_render_function.h"
#include "Vulkan/vulkan_pipeline.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/RenderDebuger.h"
#include "Renderer/RenderPassManager.h"
#include "Renderer/MaterialTemplateManager.h"
#include "Renderer/RenderQueue.h"
#include "function/Scene.h"
#include <algorithm>
#include <cassert>

namespace Render {

	RenderPass::RenderPass(const PassDesc& desc) : mRenderPass(nullptr), mPassDesc(desc)
	{
		auto ctx = RenderSystem::instance()->getRenderContext();
		mRenderPass = createRsRenderPassVk(ctx, mPassDesc);
	}

	RenderPass::RenderPass(const Name& passName, const PassDesc& desc) : RenderPass(desc)
	{
		addLogicalPass(passName, 0);
	}

	RenderPass::RenderPass(const std::vector<LogicPassDesc>& logicPassDesc, const PassDesc& desc) : RenderPass(desc)
	{
		for (const auto& logicPass : logicPassDesc) {
			this->addLogicalPass(logicPass.logicPassName,logicPass.priority,logicPass.filterMask);
		}
	}

	Render::StageMacroPairs RenderPass::getPassStageShaderMacro(const Name& logicPassName)
	{
		return 
		{ 
			{	
				ShaderStage::Vertex,	{
					{logicPassName.str(),""}
				}
			},
			{
				ShaderStage::Fragment,	{
					{logicPassName.str(),""}
				}
			}
		};
	}

	void RenderPass::addLogicalPass(const Name& name, int priority, u64 filterMask)
	{
		if (hasLogicalPass(name)) return;

		mLogicalPasses.push_back({ name, priority, filterMask });

		std::sort(mLogicalPasses.begin(), mLogicalPasses.end(), [](const LogicalPass& a, const LogicalPass& b) {
			return a.priority < b.priority;
			});
	}

	void RenderPass::removeLogicalPass(const Name& name)
	{
		auto it = std::remove_if(mLogicalPasses.begin(), mLogicalPasses.end(), [&](const LogicalPass& lp) {
			return lp.name == name;
			});
		if (it != mLogicalPasses.end()) {
			mLogicalPasses.erase(it, mLogicalPasses.end());
		}
	}

	void RenderPass::setLogicalPassPriority(const Name& name, int priority)
	{
		for (auto& lp : mLogicalPasses) {
			if (lp.name == name) {
				lp.priority = priority;
				break;
			}
		}
		std::sort(mLogicalPasses.begin(), mLogicalPasses.end(), [](const LogicalPass& a, const LogicalPass& b) {
			return a.priority < b.priority;
			});
	}

	bool RenderPass::hasLogicalPass(const Name& name) const
	{
		return std::any_of(mLogicalPasses.begin(), mLogicalPasses.end(), [&](const LogicalPass& lp) {
			return lp.name == name;
			});
	}

	const Name& RenderPass::getPassName() const
	{
		static const Name emptyName("");
		return mLogicalPasses.empty() ? emptyName : mLogicalPasses[0].name;
	}

	void RenderPass::setEntityFilterFlag(u64 flags)
	{
		if (!mLogicalPasses.empty()) {
			mLogicalPasses[0].filterMask = flags;
		}
	}

	void RenderPass::setRenderTarget(rs_rendertarget* renderTarget)
	{
		using namespace Vulkan;
		if (renderTarget == nullptr) {
			mRendertarget = nullptr;
			return;
		}

		Rect2D renderArea = {};
		if (mNextRenderAreaSet) {
			mNextRenderAreaSet = false;
			renderArea = mViewportRect;
		}

		auto ctx = RenderSystem::instance()->getRenderContext();
		if (!RenderSystem::instance()->isRenderTargetCompatibleToRenderPass(mRenderPass, renderTarget)) {
			assert(0);
			return;
		}
		this->mRendertarget = renderTarget;
	}


	void RenderPass::draw(rs_commandbuffer* cmdbuffer, Camera* cam)
	{
		if (mLogicalPasses.empty()) return;

		auto camDrawData = RenderSystem::instance()->getCurCameraDrawData();
		if (camDrawData) {
			RenderSystem::instance()->transitDrawdataResourceState(cmdbuffer, PipelineType::Graphics, camDrawData);
		}
		auto currentSceneDrawData = Scene::getCurrentScene() ? Scene::getCurrentScene()->getSceneDrawData() : nullptr;
		if (currentSceneDrawData) {
			RenderSystem::instance()->transitDrawdataResourceState(cmdbuffer, PipelineType::Graphics, currentSceneDrawData);
		}

		struct RenderBatch {
			Name passName;
			std::vector<RenderPack> packs;
			bool hasCustomViewport;
			Rect2D viewportRect;
		};
		std::vector<RenderBatch> batches;
		batches.reserve(mLogicalPasses.size());

		for (const auto& logicalPass : mLogicalPasses) {
			RenderBatch batch{
				.passName = logicalPass.name,
				.hasCustomViewport = logicalPass.hasCustomViewport,
				.viewportRect = logicalPass.viewportRect
			};
			collectRenderEntitiesForName((cam->getRenderQueue()), logicalPass.name, logicalPass.filterMask, batch.packs);

			for (auto&& pack : batch.packs) {
				if (pack.pass) {
					RenderSystem::instance()->updateParameters(cmdbuffer, pack.entity, pack.pass);
				}
			}
			batches.push_back(std::move(batch));
		}

		RenderSystem::instance()->excutePendingBufferCopies(cmdbuffer);

		RenderSystem::instance()->cmdBeginRenderPass(cmdbuffer, mRenderPass, mClrColor, mDsClear);

		if (mRendertarget) {
			Rect2D nextRenderArea{};
			RenderSystem::instance()->cmdSetRendertarget(cmdbuffer, mRendertarget, nextRenderArea);

			updateViewportAndScissor(cmdbuffer, mRendertarget);
		}
		float nearZ = 0.;
		float farZ = 1.;
		RenderSystem::instance()->getGlobalViewportZRange(nearZ, farZ);
		for (auto& batch : batches) {
			RenderMarker phaseMarker(cmdbuffer, batch.passName.c_str(), 0.2f, 0.7f, 0.9f, 1.f);
			if (batch.hasCustomViewport && mRendertarget) {
				auto renderSys = RenderSystem::instance();
				for (size_t i = 0; i < mRendertarget->m_attachments.size(); i++) {
					renderSys->cmdSetViewport(cmdbuffer, static_cast<uint32_t>(i), nearZ, farZ, batch.viewportRect);
					renderSys->cmdSetScissor(cmdbuffer, static_cast<uint32_t>(i), batch.viewportRect);
				}
				if (mRendertarget->m_depthStencilAttachment) {
					uint32_t dsIndex = static_cast<uint32_t>(mRendertarget->m_attachments.size());
					renderSys->cmdSetViewport(cmdbuffer, dsIndex, nearZ, farZ, batch.viewportRect);
					renderSys->cmdSetScissor(cmdbuffer, dsIndex, batch.viewportRect);
				}
			}

			mRenderPacks = std::move(batch.packs);
			drawImpl(cmdbuffer);
			mRenderPacks.clear();
		}

		RenderSystem::instance()->cmdEndRenderPass(cmdbuffer);
	}

	void RenderPass::collectRenderEntitiesForName(RenderQueue* renderQueue, const Name& passName, u64 renderMask, std::vector<RenderPack>& packs)
	{
		auto view = renderQueue->getView(renderMask);
		while (true) {
			auto renderData = view.next();
			if (renderData == nullptr) break;
			auto pass = renderData->entity->getPass(passName);
			if (!pass)continue;
			RenderPack pack{
				.entity = renderData->entity,
				.pass = pass
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

	void RenderPass::setNextMarkColorAndName(const vec4& color, const std::string& name)
	{
		mNextColorMarked = true;
		mNextMarkColor = color;
		mNextMarkName = name;
	}

	void RenderPass::setNextViewport(Rect2D rect2d)
	{
		mNextRenderAreaSet = true;
		mNextViewportSet = true;
		mViewportRect = rect2d;
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

		for (size_t i = 0; i < rtA->m_attachments.size(); ++i) {
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
		mRenderPass = nullptr;
	}

	void RenderPass::updateViewportAndScissor(rs_commandbuffer* cmdbuffer, rs_rendertarget* rt)
	{
		Rect2D rect{};
		rect.l = 0.f;
		rect.t = 0.f;
		rect.b = 1.f;
		rect.r = 1.f;

		if (mNextViewportSet) {
			rect = mViewportRect;
			mNextViewportSet = false;
		}

		if (rt) {
			auto renderSys = RenderSystem::instance();
			for (size_t i = 0; i < rt->m_attachments.size(); i++) {
				renderSys->cmdSetViewport(cmdbuffer, static_cast<uint32_t>(i), 0.0f, 1.f, rect);
				renderSys->cmdSetScissor(cmdbuffer, static_cast<uint32_t>(i), rect);
			}
			if (rt->m_depthStencilAttachment) {
				renderSys->cmdSetViewport(cmdbuffer, static_cast<uint32_t>(rt->m_attachments.size()), 0.0f, 1.f, rect);
				renderSys->cmdSetScissor(cmdbuffer, static_cast<uint32_t>(rt->m_attachments.size()), rect);
			}
		}
	}
}