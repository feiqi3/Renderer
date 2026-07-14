#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include "common/CoreDefs.h"
#include "common/Name.h"
#include "Renderer/Camera.h"
#include "render_resource_createinfo.h"  
#include <string>
#include <vector>

namespace Render {
	struct rs_renderpass;
	struct rs_rendertarget;
	struct rs_commandbuffer;
	enum class PassDrawOrder {
		None,
		FromFarToNear,
		FromNearToFar
	};
	struct LogicalPass {
		Name name;
		int priority = 0;
		u64 filterMask = 0xFFFFFFFFFFFFFFFF;

		bool hasCustomViewport = false;
		Rect2D viewportRect{};
		StageMacroPairs passMacros;
		PassDrawOrder drawOrder = PassDrawOrder::None;
	};

	struct LogicPassDesc {
		Name logicPassName;
		int priority;
		u64 filterMask = 0xFFFFFFFFFFFFFFFF; //RenderMask::
		PassDrawOrder drawOrder = PassDrawOrder::None;
		StageMacroPairs passMacros;
	};

	class RenderPass {
	public:
		RenderPass(const PassDesc& desc);
		RenderPass(const Name& passName, const PassDesc& desc);
		RenderPass(const std::vector<LogicPassDesc>& passName, const PassDesc& desc);
		
		virtual StageMacroPairs	getPassStageShaderMacro(const Name& logicPassName);
		virtual void init() {}
		virtual ~RenderPass();
		virtual void updateViewportAndScissor(rs_commandbuffer* cmdbuffer, rs_rendertarget* rt);
		rs_renderpass* getRaw() const { return mRenderPass; }
		void setRenderTarget(rs_rendertarget* renderTarget);
		virtual void draw(rs_commandbuffer* cmdbuffer,Camera* cam);
		virtual void preDraw(rs_commandbuffer* cmdbuffer, Camera* cam);
		inline void setClearData(const std::vector<ClearColor>& clrColor, ClearDepthStencil dsClear) {
			mClrColor = clrColor;
			mDsClear = dsClear;
		}

		struct RenderPack {
			class RenderEntity* entity;
			class Pass* pass;
		};

		void addLogicalPass(const LogicPassDesc& desc);
		void removeLogicalPass(const Name& name);
		void setLogicalPassPriority(const Name& name, int priority);
		bool hasLogicalPass(const Name& name) const;
		const std::vector<LogicalPass>& getLogicalPasses() const { return mLogicalPasses; }

		virtual void beginFrame(uint64_t frame) {};

		const Name& getPassName() const;

		void setNextMarkColorAndName(const vec4& color, const std::string& name);
		void setNextViewport(Rect2D rect2d);
		void setEntityFilterFlag(u64 flags);
		
	protected:
		std::vector<RenderPack> mRenderPacks;
		std::vector<LogicalPass> mLogicalPasses;

		void collectRenderEntitiesForName(RenderQueue* renderQueue,const LogicalPass& logicPass, std::vector<RenderPack>& packs);

	protected:
		friend class RenderPassManager;
		virtual void drawImpl(rs_commandbuffer* cmdbuffer, Camera* camera);
		bool needRebuildPipeline(rs_rendertarget* oldrt, rs_rendertarget* newrt);
		bool isRTCompatible(rs_rendertarget* rtA, rs_rendertarget* rtB);

	protected:
		rs_rendertarget* mRendertarget = nullptr;
		rs_renderpass* mRenderPass;
		const PassDesc mPassDesc;
		std::vector<ClearColor> mClrColor;
		ClearDepthStencil mDsClear;

		Rect2D mViewportRect;
		bool mNextViewportSet = false;
		bool mNextRenderAreaSet = false;

		vec4 mNextMarkColor;
		std::string mNextMarkName;
		bool mNextColorMarked = false;
	};
}

#endif