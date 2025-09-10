#include "Renderer/RenderSystem.h"
#include "vulkan/vulkan_render_function.h"
#include "window/render_resource_window_glfw.h"
#include "Renderer/RenderDataAreana.h"
#include "vulkan/vulkan_pipeline.h"
namespace Render{

	struct CommandPair {
		rs_commandbuffer* commandBuffer;
		std::vector<rs_semaphore*> wait;
		std::vector<rs_semaphore*> singal;
		rs_fence* fence = nullptr;
	};

	class RenderSystemPrivate {
	public:

		std::unique_ptr<RenderDataArena> mArena;
		std::vector<CommandPair> mRenderThreadCommandBuffers;
		std::vector<CommandPair> mLogicThreadCommandBuffers;
		std::vector<rs_semaphore*> semphoresToWait;
		std::vector<rs_semaphore*> semphoresToSignal;
		std::vector<rs_fence*> fenceToWait;
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
		sRenderSystem->mDp->mArena = std::make_unique<RenderDataArena>(1024 * 64, sRenderSystem->mBackEndContext->maxSwapChainImages);
	}
	void RenderSystem::destroyRenderSystem()
	{
		using namespace Vulkan;
		if (!sRenderSystem)return;
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
		while (1 && mUseRenderThread) {
			auto ctx = getRenderContext();
			//Wait For Logic Frame Ready
			while (!getRenderContext()->canRenderNextFrame);
			beginRsRenderFrameVk(ctx);
			auto curFif = ctx->curRenderFrame % ctx->maxFrameInFlight;
			auto nxtImg = waitForNextPresentImage(ctx, mDp->semphoresToWait[curFif], 0);
			for (auto&& cmd : mDp->mRenderThreadCommandBuffers) {
				Vulkan::cmdSubmitCmdBuffer(getRenderContext(), (rs_commandbuffer_vk*)cmd.commandBuffer, QueueType_Graphics, cmd.wait, cmd.singal, cmd.fence);
			}
			submitToPresentImage(ctx, nxtImg, { mDp->semphoresToSignal[curFif] });
			ctx->canRenderNextFrame = false;
		}


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
	rs_pipeline* RenderSystem::createRenderPipeline(rs_renderpass* renderpass, PipelineDesc& pipelineDescription)
	{
		return Vulkan::createRsPipeline(getRenderContext(), (Vulkan::rs_renderpass_vk*)renderpass, pipelineDescription);
	}
	void RenderSystem::destroyRenderPipeline(rs_pipeline* pipeline)
	{
		auto plVk = (Vulkan::rs_pipeline_vk*)pipeline;
		Vulkan::destroyRsPipeline(getRenderContext(), plVk);
	}
	rs_image* RenderSystem::createImage2D(void* data, size_t byteSize, ImageFormat format, int x, int y, int z, int layer, int layersize, int mipmap)
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
			Vulkan::updateImage(getRenderContext(), ret, data, byteSize, 0, 0, 0, x, y, z, 0,layer,layersize,true );
		}
		return ret;

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
	void RenderSystem::updateUniformBufferData(rs_binding_pos binding, void* data, uint32_t size, RenderInfo& info)
	{	
		auto ctx = getRenderContext();
		Vulkan::updateDrawData(ctx, ctx->nextRenderFrame, (Vulkan::rs_pipeline_vk*)info.pipeline, (Vulkan::rs_drawdata_vk*)info.drawData, binding, data, size);
	}
	void RenderSystem::updateUniform(rs_binding_pos binding, rs_buffer* buffer, RenderInfo& info)
	{
		auto ctx = getRenderContext();
		Vulkan::updateDrawData(ctx, ctx->nextRenderFrame, (Vulkan::rs_pipeline_vk*)info.pipeline, (Vulkan::rs_drawdata_vk*)info.drawData, binding,(Vulkan::rs_buffer_vk*) buffer);
	}
	void RenderSystem::updateUniform(rs_binding_pos binding, rs_image* image, RenderInfo& info)
	{
		auto ctx = getRenderContext();
		Vulkan::updateDrawData(ctx, ctx->nextRenderFrame, (Vulkan::rs_pipeline_vk*)info.pipeline, (Vulkan::rs_drawdata_vk*)info.drawData, binding, (Vulkan::rs_image_vk*)image);

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
		CommandPair pair{
			.commandBuffer = cmdBuffer,
			.wait = wait,
			.singal = singal,
			.fence = fence
		};
		mDp->mLogicThreadCommandBuffers.push_back(pair);
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