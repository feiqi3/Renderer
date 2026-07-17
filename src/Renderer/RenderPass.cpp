#include "Renderer/RenderPass.h"
#include "Vulkan/vulkan_render_function.h"
#include "Vulkan/vulkan_pipeline.h"
#include "render_function.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/RenderDebuger.h"
#include "Renderer/RenderPassManager.h"
#include "Renderer/MaterialTemplateManager.h"
#include "Renderer/MaterialInstance.h"
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
		LogicPassDesc logicPassDesc;
		logicPassDesc.logicPassName = passName;
		logicPassDesc.priority = 0;
		addLogicalPass(logicPassDesc);
	}

	RenderPass::RenderPass(const std::vector<LogicPassDesc>& logicPassDesc, const PassDesc& desc) : RenderPass(desc)
	{
		for (const auto& logicPass : logicPassDesc) {
			this->addLogicalPass(logicPass);
		}
	}

	Render::StageMacroPairs RenderPass::getPassStageShaderMacro(const Name& logicPassName)
	{
		for (const auto& logicPass : mLogicalPasses) {
			if (logicPass.name == logicPassName) {
				auto& macro =  logicPass.passMacros;
				StageMacroPairs passNameMacro = {
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
				return mergeStageMacroPairs(macro, passNameMacro);
			}
		}

		return 
		{ 

		};
	}

	void RenderPass::addLogicalPass(const LogicPassDesc& desc)
	{
		if (hasLogicalPass(desc.logicPassName)) return;
		LogicalPass logicPass{};
		logicPass.name = desc.logicPassName;
		logicPass.filterMask = desc.filterMask;
		logicPass.priority = desc.priority;
		logicPass.passMacros = desc.passMacros;
		logicPass.drawOrder = desc.drawOrder;
		mLogicalPasses.push_back(logicPass);

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

	float RenderPass::getLogicPassTimes(const Name& name) const
	{
		std::vector<std::pair<Render::Name, float>> times;
		for (const auto& logicPass : mLogicalPasses) {
			if (logicPass.name == name) {
				return logicPass.lastLogicPassTime;
			}
		}
		return -1;
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
		this->passBegin();
		preDraw(cmdbuffer, cam);
		if (mLogicalPasses.empty()) return;
		if (!mRendertarget) {
			return;
		}
		if (cam) {
			RenderSystem::instance()->setCurrentCamera(cam);
			auto camDrawData = RenderSystem::instance()->getCurCameraDrawData();
			if (camDrawData) {
				RenderSystem::instance()->transitDrawdataResourceState(cmdbuffer, PipelineType::Graphics, camDrawData);
			}
		}
		auto scene = Scene::getCurrentScene();
		if (scene) {
			RenderSystem::instance()->transitDrawdataResourceState(cmdbuffer, PipelineType::Graphics, scene->getSceneDrawData());
			RenderSystem::instance()->transitDrawdataResourceState(cmdbuffer, PipelineType::Graphics, scene->getSceneShadowData());
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
			logicPassBegin(logicalPass.name);
			RenderBatch batch{
				.passName = logicalPass.name,
				.hasCustomViewport = logicalPass.hasCustomViewport,
				.viewportRect = logicalPass.viewportRect
			};
			//This can do a parallel?
			collectRenderEntitiesForName((cam->getRenderQueue()), logicalPass, batch.packs);

			static auto getEntityDistToCam = [](const vec3& camPos, RenderEntity* entity) -> float {
				auto worldPos = entity->getWorldPos();
				return length(camPos - worldPos);
			};


			if (cam != nullptr) {

				const auto& camPos = cam->getPosition();

				switch (logicalPass.drawOrder)
				{
				case PassDrawOrder::None:
				{
					std::sort(batch.packs.begin(), batch.packs.end(), [](RenderPack& a, RenderPack& b) {
						auto priorityA = a.entity->getMaterial()->getRenderOrder();
						auto priorityB = b.entity->getMaterial()->getRenderOrder();
						return priorityA < priorityB;
						});
					break;
				}
				case PassDrawOrder::FromFarToNear:
				{
					std::sort(batch.packs.begin(), batch.packs.end(), [&camPos]( RenderPack& a, RenderPack& b) {
						float distToCamA = getEntityDistToCam(camPos, a.entity);
						float distToCamB = getEntityDistToCam(camPos, b.entity);
						return distToCamA >= distToCamB;
						});
					break;
				}
				case PassDrawOrder::FromNearToFar:
				{
					std::sort(batch.packs.begin(), batch.packs.end(), [&camPos](RenderPack& a, RenderPack& b) {
						float distToCamA = getEntityDistToCam(camPos, a.entity);
						float distToCamB = getEntityDistToCam(camPos, b.entity);
						return distToCamA <= distToCamB;
						});
					break;

				}
				default:break;
				}
			}

			for (auto&& pack : batch.packs) {
				if (pack.pass) {
					RenderSystem::instance()->updateParameters(cmdbuffer, pack.entity, pack.pass);
				}
			}
			batches.emplace_back(std::move(batch));
			logicPassEnd(logicalPass.name);
		}

		RenderSystem::instance()->excutePendingBufferCopies(cmdbuffer);


		RenderSystem::instance()->cmdBeginRenderPass(cmdbuffer, mRenderPass, mClrColor, mDsClear);
		Rect2D nextRenderArea{};
		RenderSystem::instance()->cmdSetRendertarget(cmdbuffer, mRendertarget, nextRenderArea);

		updateViewportAndScissor(cmdbuffer, mRendertarget);
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
			drawImpl(cmdbuffer,cam);
			mRenderPacks.clear();
		}
		RenderSystem::instance()->cmdEndRenderPass(cmdbuffer);
		this->passEnd();
	}

	void RenderPass::preDraw(rs_commandbuffer* cmdbuffer, Camera* cam)
	{
		return;
	}

	void RenderPass::collectRenderEntitiesForName(RenderQueue* renderQueue, const LogicalPass& logicPass, std::vector<RenderPack>& packs)
	{
		auto view = renderQueue->getView(logicPass.filterMask);
		const auto& passName = logicPass.name;
		while (true) {
			auto renderData = view.next();
			if (renderData == nullptr) break;
			auto pass = renderData->entity->getPass(passName);
			if (!pass)continue;
			RenderPack pack{
				.entity = renderData->entity,
				.pass = pass,
			};
			packs.push_back(pack);
		}
	}

	float RenderPass::getPassTime()
	{
		return mRenderPassLogicTime;
	}

	void RenderPass::drawImpl(rs_commandbuffer* cmdbuffer, Camera* camera)
	{
		for (auto& pack : mRenderPacks) {
			RenderSystem::instance()->drawIndexed(cmdbuffer, pack.entity, camera, pack.pass);
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

	void RenderPass::logicPassBegin(const Name& name)
	{
		for (auto& logicPass : mLogicalPasses) {
			if (logicPass.name == name) {
				logicPass.passBeginTime = std::chrono::steady_clock::now();
			}
		}
	}

	void RenderPass::logicPassEnd(const Name& name)
	{
		for (auto& logicPass : mLogicalPasses) {
			if (logicPass.name == name) {
				auto dur = std::chrono::steady_clock::now() - logicPass.passBeginTime;
				logicPass.lastLogicPassTime = std::chrono::duration_cast<std::chrono::microseconds>(dur).count() / 1000.f;
			}
		}
	}

	void RenderPass::passBegin()
	{
		mRenderPassBeginTimePoint = std::chrono::steady_clock::now();
	}

	void RenderPass::passEnd()
	{
		auto dur = std::chrono::steady_clock::now() - mRenderPassBeginTimePoint;

		mRenderPassLogicTime = std::chrono::duration_cast<std::chrono::microseconds>(dur).count() / 1000.f;
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