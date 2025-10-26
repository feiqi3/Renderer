#include "Renderer/RenderSystem.h"
#include "vulkan/vulkan_render_function.h"
#include "window/render_resource_window_glfw.h"
#include "Renderer/RenderDataAreana.h"
#include "vulkan/vulkan_pipeline.h"
#include "Renderer/MaterialVarient.h"
#include "vulkan/vulkan_command.h"
#include "Renderer/RenderPassManager.h"
namespace Render{

	struct CommandPair {
		rs_commandbuffer* commandBuffer;
		std::vector<rs_semaphore*> wait;
		std::vector<rs_semaphore*> singal;
		rs_fence* fence = nullptr;
	};

	class RenderSystemPrivate {
	public:
		std::unique_ptr< RenderPassManager> mPassManager;
		std::unique_ptr<RenderDataArena> mArena;
		std::vector<CommandPair> mRenderThreadCommandBuffers;
		std::vector<CommandPair> mLogicThreadCommandBuffers;
		std::vector<rs_semaphore*> semphoresToWait;
		std::vector<rs_semaphore*> semphoresToSignal;
		std::vector<rs_fence*> fenceToWait;
		std::vector<rs_rendertarget*> mSwapchainRT;
	public:
		void* updateFramePendingData(uint32_t fif, uint64_t frame, void* data, uint32_t size);
		void cleanUpFramesPendingData(uint32_t fif, uint64_t frame);
	};

	RenderSystem* RenderSystem::sRenderSystem = nullptr;
	void RenderSystem::createRenderSystem(const BackEndInitDesc& backEndDesc, Window::rs_window* window)
	{
		using namespace Window;
		using namespace Vulkan;
		if (sRenderSystem) {
			return;
		}
		sRenderSystem = new RenderSystem;
		sRenderSystem->mWindow = window;
		sRenderSystem->mBackEndContext = initVulkanBackEnd(backEndDesc, window);
		for (int i = 0; i < sRenderSystem->getRenderContext()->maxFrameInFlight; ++i) {
			auto render_context = sRenderSystem->getRenderContext();
			sRenderSystem->mDp->fenceToWait.push_back(createRsFence(render_context));
			sRenderSystem->mDp->fenceToWait.push_back(createRsFence(render_context));
			sRenderSystem->mDp->fenceToWait.push_back(createRsFence(render_context));
			resetRsFence(render_context, sRenderSystem->mDp->fenceToWait[i]);
			sRenderSystem->mDp->semphoresToWait.push_back(createRsSemaphore(render_context));
			sRenderSystem->mDp->semphoresToSignal.push_back(createRsSemaphore(render_context));
		}
		sRenderSystem->mDp->mPassManager = std::make_unique<RenderPassManager>();
		sRenderSystem->mDp->mArena = std::make_unique<RenderDataArena>(1024 * 64, sRenderSystem->mBackEndContext->maxSwapChainImages);
		sRenderSystem->mCurLogicFrameInFlight = sRenderSystem->mBackEndContext->maxFrameInFlight;
	}
	void RenderSystem::destroyRenderSystem()
	{
		using namespace Vulkan;
		if (!sRenderSystem)return;
		sRenderSystem->mDp->mPassManager = 0;
		deinitVulkanBackEnd((rs_context_vk*)sRenderSystem->mBackEndContext);
		sRenderSystem->mWindow = 0;
		sRenderSystem->mBackEndContext = 0;
		delete sRenderSystem;
		sRenderSystem = 0;
	}
	RenderSystem* RenderSystem::instance()
	{
		return sRenderSystem;
	}
	void RenderSystem::BeginRenderFrame()
	{
		using namespace Vulkan;
		auto ctx = getRenderContext();
		auto curFif = ctx->curRenderFrame % ctx->maxFrameInFlight;
		if (mUseRenderThread) {
			while (1) {
				//Wait For Logic Frame Ready
				while (!getRenderContext()->canRenderNextFrame);
				beginRsRenderFrameVk(ctx);
				auto nxtImg = waitForNextPresentImage(ctx, mDp->semphoresToWait[curFif], 0);
				for (auto&& cmd : mDp->mRenderThreadCommandBuffers) {
					Vulkan::cmdSubmitCmdBuffer(getRenderContext(), (rs_commandbuffer_vk*)cmd.commandBuffer, QueueType_Graphics, cmd.wait, cmd.singal, cmd.fence);
				}
				submitToPresentImage(ctx, nxtImg, { mDp->semphoresToSignal[curFif] });
				ctx->canRenderNextFrame = false;
			}
		}
		else {
			beginRsRenderFrameVk(ctx);
			auto nxtImg = waitForNextPresentImage(ctx, mDp->semphoresToWait[curFif], 0);
			for (auto&& cmd : mDp->mRenderThreadCommandBuffers) {
				Vulkan::cmdSubmitCmdBuffer(getRenderContext(), (rs_commandbuffer_vk*)cmd.commandBuffer, QueueType_Graphics, cmd.wait, cmd.singal, cmd.fence);
			}
			submitToPresentImage(ctx, nxtImg, { mDp->semphoresToSignal[curFif] });
		}


	}
	RenderPassManager* RenderSystem::getRenderPassManager() const
	{
		return mDp->mPassManager.get();
	}

	RenderPass* RenderSystem::getRenderPass(const std::string& pass) 
	{
		return getRenderPassManager()->getRenderPass(pass);
	}

	rs_commandbuffer* RenderSystem::GetCommandBufferCurFrameCurThread()
	{
		auto ctx = getRenderContext();
		return ctx->cmdBufferMgr->getCmdBufferLocalThread(ctx, getNextRenderFrame(), QueueType_Graphics, false);
	}
	rs_renderpass* RenderSystem::createRenderPass(rs_rendertarget* renderTarget, PassDesc& passDescription)
	{
		return Vulkan::createRsRenderPassVk(this->getRenderContext(), (Render::Vulkan::rs_rendertarget_vk*)renderTarget, passDescription);
	}
	void RenderSystem::destoyRenderPass(rs_renderpass* renderPass)
	{
		Vulkan::rs_renderpass_vk* rpVk = (Vulkan::rs_renderpass_vk*)renderPass;
		Vulkan::destroyRsRenderPassVk(this->getRenderContext(), rpVk);
	}
	void RenderSystem::cmdBeginRenderPass(rs_commandbuffer* cmdbuf, rs_renderpass* pass,std::vector<ClearColor>& clearColor, ClearDepthStencil& clearDs)
	{
		Vulkan::cmdBeginRenderPass((Vulkan::rs_commandbuffer_vk*)cmdbuf, (Vulkan::rs_renderpass_vk*)pass, clearColor, clearDs);
		cmdbuf->currentRenderPass = pass;
	}
	void RenderSystem::cmdEndRenderPass(rs_commandbuffer* cmdbuf)
	{
		Vulkan::cmdEndRenderPass((Vulkan::rs_commandbuffer_vk*)cmdbuf);
	}

	void RenderSystem::cmdSetScissor(rs_commandbuffer* cmdbuf, int framebufferIdx, const Rect2D& rect)
	{
		Vulkan::cmdSetScissor((Vulkan::rs_commandbuffer_vk*)cmdbuf, rect, framebufferIdx);
	}

	void RenderSystem::cmdSetViewport(rs_commandbuffer* cmdbuf, int framebufferIdx, float minDepth, float maxDepth, const Rect2D& rect)
	{
		Vulkan::cmdSetViewport((Vulkan::rs_commandbuffer_vk*)cmdbuf, rect,minDepth,maxDepth,framebufferIdx);
	}

	rs_buffer* RenderSystem::createBuffer(void* data, uint32_t size, const BufferDesc& desc)
	{
		rs_buffer* buffer = Vulkan::createRsBuffer(getRenderContext(), desc);
		if (desc.mappable) {
			Vulkan::mapRsBuffer(getRenderContext(), (Vulkan::rs_buffer_vk*)buffer);
		}
		updateBuffer(buffer, data, size, 0, true);
		return buffer;
	}


	void RenderSystem::destroyBuffer(rs_buffer* buffer)
	{
		auto rsBuffer = (Vulkan::rs_buffer_vk*)buffer;
		Vulkan::destroyRsBuffer(getRenderContext(), rsBuffer, false);
	}
	void RenderSystem::updateBuffer(rs_buffer* buffer, void* data, uint32_t size, uint32_t offset, bool imm)
	{
		if (buffer->mappedPtr) {
			memcpy(buffer->mappedPtr, data, size);
			return;
		}
		Vulkan::updateBuffer(getRenderContext(), (Vulkan::rs_buffer_vk*)buffer, data, size, offset, imm);
	}
	rs_pipeline* RenderSystem::createRenderPipeline(rs_renderpass* renderpass, PipelineDesc& pipelineDescription)
	{
		return Vulkan::createRsPipeline(getRenderContext(), (Vulkan::rs_renderpass_vk*)renderpass, pipelineDescription);
	}
	void RenderSystem::destroyRenderPipeline(rs_pipeline* pipeline)
	{
		auto plVk = (Vulkan::rs_pipeline_vk*)pipeline;
		Vulkan::destroyRsPipeline(getRenderContext(), plVk);
	}
	rs_sampler* RenderSystem::createSampler(const SamplerDesc& desc)
	{
		auto ctx = getRenderContext();
		return Vulkan::createRsSampler(ctx, desc);
	}
	void RenderSystem::destroyRsSampler(rs_sampler* sampler)
	{
		auto ctx = getRenderContext();
		auto vkSampler = (Vulkan::rs_sampler_vk*)sampler;
		Vulkan::destroyRsSampler(ctx, vkSampler);
	}
	void RenderSystem::clearDrawData(rs_drawdata* drawdata)
	{
		return Vulkan::clearDrawData(getRenderContext(),(Vulkan::rs_drawdata_vk*)drawdata);
	}
	void RenderSystem::destroyDrawData(rs_drawdata* drawdata)
	{
		return Vulkan::destroyDrawData(getRenderContext(),  (Vulkan::rs_drawdata_vk*)drawdata);
	}
	rs_drawdata* RenderSystem::createDrawData()
	{
		return Vulkan::createDrawData(getRenderContext());
	}
	rs_image* RenderSystem::createImage2D(void* data, size_t byteSize, ImageFormat format, int x, int y, int z, int layer, int mipmap)
	{
		ImageDesc desc{};
		desc.width = x;
		desc.height = y;
		desc.depth = z;
		desc.mipLevels = 1;
		desc.arrayLayers = layer;

		ImageType type = ImageType::V2D;
		if (z > 1) {
			type = ImageType::V3D;
		}
		else if (layer > 1) {
			type = ImageType::V2D_Array;
		}

		desc.type = type;
		desc.format = format;
		desc.usage = ImageUsage::ImageUsage_Sampled;

		auto ret = Vulkan::createRsImage(getRenderContext(), desc);
		if (data) {
			Vulkan::updateImage(getRenderContext(), ret, data, byteSize, 0, 0, 0, x, y, z,0, 0,layer,true );
		}
		return ret;

	}
	rs_image* RenderSystem::createRTTexture(ImageFormat format, int x, int y, int z, int layer, bool needSample)
	{
		ImageDesc desc{};
		desc.width = x;
		desc.height = y;
		desc.depth = z;
		desc.mipLevels = 1;
		desc.arrayLayers = layer;

		ImageType type = ImageType::V2D;
		if (z > 1) {
			type = ImageType::V3D;
		}
		else if (layer > 1) {
			type = ImageType::V2D_Array;
		}

		desc.type = type;
		desc.format = format;
		desc.usage = ImageUsage::ImageUsage_ColorAttachment;

		if (needSample) {
			desc.usage |= ImageUsage::ImageUsage_Sampled;
		}

		return Vulkan::createRsImage(getRenderContext(), desc);
	}
	rs_image* RenderSystem::createDepthStencilTexture(ImageFormat format, int x, int y,bool needSample)
	{
		ImageDesc desc{};
		desc.width = x;
		desc.height = y;
		desc.depth = 1;
		desc.mipLevels = 1;
		desc.arrayLayers = 1;

		ImageType type = ImageType::V2D;

		desc.type = type;
		desc.format = format;
		
		desc.usage = ImageUsage::ImageUsage_DepthStencilAttachment;
		if (needSample) {
			desc.usage |= ImageUsage::ImageUsage_Sampled;
		}
		return Vulkan::createRsImage(getRenderContext(), desc);
	}
	rs_rendertarget* RenderSystem::createRendertarget(const std::vector<rs_image*>images, rs_image* dsTex)
	{
		auto ctx = getRenderContext();
		return Vulkan::createRsRenderTarget(ctx, (Vulkan::rs_image_vk**)images.data(), images.size(), (Vulkan::rs_image_vk*)dsTex);
	}
	void RenderSystem::destroyRenderTarget(rs_rendertarget* rt)
	{
		auto ctx = getRenderContext();
		Vulkan::rs_rendertarget_vk* vkrt = (Vulkan::rs_rendertarget_vk*)rt;
		Vulkan::destroyRsRenderTarget(ctx, vkrt);
	}
	void RenderSystem::beginFrame()
	{
		Vulkan::beginRsFrameVk(getRenderContext());
		std::swap(mDp->mRenderThreadCommandBuffers, mDp->mLogicThreadCommandBuffers);
		mDp->mLogicThreadCommandBuffers.clear();
		currentLogicFrame++;
		mCurLogicFrameInFlight = currentLogicFrame % getRenderContext()->maxFrameInFlight;
		mDp->cleanUpFramesPendingData(currentLogicFrame % mBackEndContext->maxFrameInFlight, currentLogicFrame);
	}

	rs_binding_pos RenderSystem::getBindingPos(const std::string& bindingName, Material* material)
	{
		auto data = material->getBindinginfoByName(bindingName);
		if (data.has_value()) {
			return (*data).bindingPos;
		}
		return INVALID_BINDING_POS;
	}

	rs_binding_pos RenderSystem::getBindingPos(const std::string& bindingName, MaterialTemplate* matTemplate, const std::string& passName)
	{
		auto varient = matTemplate->getVarient(passName);
		if (varient) {
			return getBindingPos(bindingName, varient);
		}
		return INVALID_BINDING_POS;
	}

	void RenderSystem::updateUniformBufferData(rs_binding_pos binding, void* data, uint32_t size, Pass* pass)
	{	
		auto ctx = getRenderContext();
		auto pipeline = (Vulkan::rs_pipeline_vk*)pass->mMaterial->getRsPipeline();
		auto drawData = (Vulkan::rs_drawdata_vk*)pass->mDrawData;
		Vulkan::updateDrawData(ctx, ctx->nextRenderFrame,pipeline,drawData, binding, data, size);
	}
	void RenderSystem::updateUniform(rs_binding_pos binding, rs_buffer* buffer, Pass* pass)
	{
		auto ctx = getRenderContext();
		auto pipeline = (Vulkan::rs_pipeline_vk*)pass->mMaterial->getRsPipeline();
		auto drawData = (Vulkan::rs_drawdata_vk*)pass->mDrawData;
		Vulkan::updateDrawData(ctx, ctx->nextRenderFrame, pipeline, drawData, binding, (Vulkan::rs_buffer_vk*)buffer);
	}
	void RenderSystem::updateUniform(rs_binding_pos binding, rs_image* image, Pass* pass)
	{
		auto ctx = getRenderContext();
		auto pipeline = (Vulkan::rs_pipeline_vk*)pass->mMaterial->getRsPipeline();
		auto drawData = (Vulkan::rs_drawdata_vk*)pass->mDrawData;
		Vulkan::updateDrawData(ctx, ctx->nextRenderFrame,pipeline,drawData, binding, (Vulkan::rs_image_vk*)image);

	}
	void RenderSystem::updateUniform(rs_binding_pos binding, rs_sampler* sampler, Pass* pass)
	{
		auto ctx = getRenderContext();
		auto pipeline = (Vulkan::rs_pipeline_vk*)pass->mMaterial->getRsPipeline();
		auto drawData = (Vulkan::rs_drawdata_vk*)pass->mDrawData;
		Vulkan::updateDrawData(ctx, ctx->nextRenderFrame, pipeline, drawData, binding, (Vulkan::rs_sampler_vk*)sampler);
	}
	void RenderSystem::updateImageData(rs_image* image, void* data,size_t byteSize, int x, int y, int z, int width, int height, int depth, int layerOffset, int layerSize, int mip)
	{
		auto ctx = getRenderContext();
		Vulkan::updateImage(ctx, (Vulkan::rs_image_vk*)image, data, byteSize, x, y, z, width, height, depth, mip, layerOffset, layerSize, false);
	}
	void RenderSystem::updateBufferData(rs_buffer* buffer, void* data, size_t byteSize, size_t dstOffset)
	{
		auto ctx = getRenderContext();
		Vulkan::updateBuffer(getRenderContext(), (Vulkan::rs_buffer_vk*)buffer, data, byteSize, dstOffset, false);
	}
	void* RenderSystem::placeFramePendingData(void* data, uint32_t size)
	{
		auto fif = currentLogicFrame % mBackEndContext->maxFrameInFlight;
		return mDp->updateFramePendingData(fif, currentLogicFrame, data, size);
	}
	void RenderSystem::submitCmdBuffer(rs_commandbuffer* cmdBuffer, std::vector<rs_semaphore*> wait, std::vector<rs_semaphore*> singal, rs_fence* fence)
	{
		if (cmdBuffer->hasCommands == false) {
			return;
		}
		cmdBuffer->hasCommands = false;
		CommandPair pair{
			.commandBuffer = cmdBuffer,
			.wait = wait,
			.singal = singal,
			.fence = fence
		};
		mDp->mLogicThreadCommandBuffers.push_back(pair);
	}
	void RenderSystem::drawIndexed(rs_commandbuffer* cmdBuffer, RenderEntity* entity,const std::string& passName)
	{
		auto pass = entity->getPass(passName);
		if (!pass)return;

		drawIndexed(cmdBuffer, entity, pass);
	}
	void RenderSystem::drawIndexed(rs_commandbuffer* cmdBuffer, RenderEntity* entity, Pass* pass)
	{
		cmdBuffer->hasCommands = true;
		auto pipeline = (Vulkan::rs_pipeline_vk*)pass->mMaterial->getRsPipeline();
		auto drawData = (Vulkan::rs_drawdata_vk*)pass->mDrawData;
		Vulkan::cmdDrawIndexed((Vulkan::rs_commandbuffer_vk*)cmdBuffer, pipeline, entity->getRenderInfo(), drawData, getCurFif());
	}
	void RenderSystem::clearRenderEntity(RenderEntity* entity)
	{
		for (auto& [passName, pass] : entity->mPasses) {
			auto drawDataVk = (Vulkan::rs_drawdata_vk*)pass->mDrawData;
			Vulkan::clearDrawData(getRenderContext(),drawDataVk);
		}
	}
	uint64_t RenderSystem::getNextRenderFrame() const
	{
		return currentLogicFrame;
	}
	uint32_t RenderSystem::getCurFif() const
	{
		return mCurLogicFrameInFlight;
	}
	rs_image* RenderSystem::getSwapchainImage(uint32_t idx)
	{
		return getRenderContext()->swapchain->swapchainImgs[idx];
	}
	rs_rendertarget* RenderSystem::getNextSwapchainRendertarget()
	{
		auto ctx = getRenderContext();
		uint32_t NxtImageIdx = this->getNextRenderFrame() % ctx->maxSwapChainImages;
		return mDp->mSwapchainRT[NxtImageIdx];
	}

	void RenderSystem::initSwapchainRT()
	{
		auto ctx = getRenderContext();
		for (int i = 0; i < ctx->maxSwapChainImages; ++i) {
			mDp->mSwapchainRT.push_back(createRendertarget({ getSwapchainImage(i) }, 0));
		}
	}

	void RenderSystem::deinitSwapchainRT()
	{
		for (auto&& rt : mDp->mSwapchainRT) {
			destroyRenderTarget(rt);
		}
		mDp->mSwapchainRT.clear();
	}

	void RenderSystem::cmdBegin(rs_commandbuffer* cmdBuffer)
	{
		Vulkan::cmdBeginRecord((Vulkan::rs_commandbuffer_vk*)cmdBuffer);
	}

	void RenderSystem::cmdEnd(rs_commandbuffer* cmdBuffer)
	{
		Vulkan::cmdEndRecord((Vulkan::rs_commandbuffer_vk*)cmdBuffer);
	}

	RenderSystem::RenderSystem()
	{
		mDp = std::make_unique< RenderSystemPrivate>();
		
	}
	RenderSystem::~RenderSystem()
	{

	}

	void* RenderSystemPrivate::updateFramePendingData(uint32_t fif, uint64_t frame, void* data, uint32_t size)
	{
		const uint32_t BufferSize = 1024 * 64; //64k

		auto allocInfo = mArena->allocateFromArena(size);
		if (allocInfo.data) {
			memcpy(allocInfo.data + allocInfo.offset, data, size);
		}
		else {
			throw std::runtime_error("No Valid Space for Render Data");
		}
		return allocInfo.data + allocInfo.offset;
	}
	void RenderSystemPrivate::cleanUpFramesPendingData(uint32_t fif,uint64_t frame)
	{
		mArena->beginFrame(frame);
	}

}