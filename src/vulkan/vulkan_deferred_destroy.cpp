#include "vulkan/vulkan_deferred_destroy.h"
#include "vulkan/vulkan_render_function.h"
#include "vulkan/vulkan_pipeline.h"
namespace Render::Vulkan {
	DeferredDestroyer::DeferredDestroyer(int maxFrameInFlight)
	{
		mMaxFrameInFlight = maxFrameInFlight;
		this->mFrameBuffers.resize(mMaxFrameInFlight);
		this->mFrameRendertargets.resize(mMaxFrameInFlight);
		this->mFrameImages.resize(mMaxFrameInFlight);
		this->mFrameSamplers.resize(mMaxFrameInFlight);
		this->mFramePipelines.resize(mMaxFrameInFlight);
		this->mFrameRenderPasses.resize(mMaxFrameInFlight);
		this->mFrameSemaphores.resize(mMaxFrameInFlight);
		this->mFrameFences.resize(mMaxFrameInFlight);
		this->mFrameDrawDatas.resize(mMaxFrameInFlight);
		this->mFrameBindlessDatas.resize(mMaxFrameInFlight);
	}
	void DeferredDestroyer::destroyDrawData(uint64_t frame, rs_drawdata_vk* drawData)
	{
		int f = frame % mMaxFrameInFlight;
		mFrameDrawDatas[f].push_back(drawData);
	}

	void DeferredDestroyer::destroyBuffer(uint64_t frame, rs_buffer_vk* buffer)
	{
		int f = frame % mMaxFrameInFlight;
		mFrameBuffers[f].push_back(buffer);
	}


	void DeferredDestroyer::destroyRenderTarget(uint64_t frame, rs_rendertarget_vk* rt)
	{
		int f = frame % mMaxFrameInFlight;
		mFrameRendertargets[f].push_back(rt);
	}

	void DeferredDestroyer::destroyImage(uint64_t frame, rs_image_vk* image)
	{
		int f = frame % mMaxFrameInFlight;
		mFrameImages[f].push_back(image);
	}

	void DeferredDestroyer::destroySampler(uint64_t frame, rs_sampler_vk* sampler)
	{
		int f = frame % mMaxFrameInFlight;
		mFrameSamplers[f].push_back(sampler);
	}

	void DeferredDestroyer::destroyPipeline(uint64_t frame, rs_pipeline* pipeline)
	{
		int f = frame % mMaxFrameInFlight;
		mFramePipelines[f].push_back(pipeline);
	}

	void DeferredDestroyer::destroyRenderPass(uint64_t frame, rs_renderpass_vk* renderpass)
	{
		int f = frame % mMaxFrameInFlight;
		mFrameRenderPasses[f].push_back(renderpass);
	}

	void DeferredDestroyer::destroySemaphores(rs_semaphore_vk* semaphores)
	{
		for (int i = 0; i < semaphores->cnt; i++) {
			VkSemaphore frameSemaphore = ((VkSemaphore*)semaphores->native)[i];
			mFrameSemaphores[i].push_back(frameSemaphore);
		}
	}

	void DeferredDestroyer::destroyFences(rs_fence_vk* fences)
	{
		for (int i = 0; i < fences->cnt; i++) {
			VkFence frameFence = ((VkFence*)fences->native)[i];
			mFrameFences[i].push_back(frameFence);
		}
	}

	void DeferredDestroyer::destroyBindlessData(uint64_t frame, rs_bindless_data_vk* data)
	{
		int f = frame % mMaxFrameInFlight;
		mFrameBindlessDatas[f].push_back(data);
	}


	void DeferredDestroyer::endFrameDestroy(rs_context_vk* ctx,uint64_t frame)
	{
		
		if (ctx->nextRenderFrame < ctx->maxFrameInFlight) {
			//Special case for first two frame. for some init case.
			return;
		}
		int f = frame % mMaxFrameInFlight;
		auto& buffers = mFrameBuffers[f];
		for (auto&& buffer : buffers) {
			if (!buffer)continue;
			destroyRsBuffer(ctx,buffer,true);
		}
		buffers.clear();

		auto& rts = mFrameRendertargets[f];
		for (auto&& rt : rts) {
			if (!rt)continue;
			destroyRsRenderTarget(ctx, rt, true);
		}
		rts.clear();

		auto& images = mFrameImages[f];
		for (auto&& image : images) {
			if (!image)continue;
			destroyRsImage(ctx, image, true);
		}
		images.clear();

		auto& samplers = mFrameSamplers[f];
		for (auto&& sampler : samplers){
			if (!sampler)continue;
			destroyRsSampler(ctx, sampler, true);
		}
		samplers.clear();

		auto& pipelines = mFramePipelines[f];
		for (auto&& pipeline : pipelines) {
			if (!pipeline)continue;
			destroyRsPipeline(ctx, pipeline, true);
		}
		pipelines.clear();

		auto& renderpasses = mFrameRenderPasses[f];
		for (auto&& renderpass : renderpasses) {
			if (!renderpass)continue;
			destroyRsRenderPassVk(ctx, renderpass, true);
		}
		renderpasses.clear();

		auto& semaphores = mFrameSemaphores[f];
		for (auto&& semaphore : semaphores) {
			VkSemaphore sem = (VkSemaphore)semaphore;
			vkDestroySemaphore(ctx->device, sem, nullptr);
		}
		semaphores.clear();

		auto& fences = mFrameFences[f];
		for (auto&& fence : fences) {
			VkFence fen = (VkFence)fence;
			vkDestroyFence(ctx->device, fen, nullptr);
		}
		fences.clear();

		auto& drawdatas = mFrameDrawDatas[f];
		for (auto&& drawdata : drawdatas) {
			clearDrawData(ctx, drawdata);
			delete drawdata;
		}
		drawdatas.clear();


		auto& bindlessData = mFrameBindlessDatas[f];
		for (auto&& data : bindlessData) {
			ctx->descriptorSetMgr->ReturnDescriptorSet(ctx, data->descriptorSet);
		
			delete data;
		}
		bindlessData.clear();
	}

	void DeferredDestroyer::clearAll(rs_context_vk* ctx)
	{
		for (int i = 0; i < mMaxFrameInFlight; ++i) {
			endFrameDestroy(ctx, i);
		}
	}

}