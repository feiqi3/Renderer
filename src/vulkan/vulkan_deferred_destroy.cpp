#include "vulkan/vulkan_deferred_destroy.h"
#include "vulkan/vulkan_render_function.h"
#include "vulkan/vulkan_pipeline.h"
namespace Render::Vulkan {
	DeferredDestroyer::DeferredDestroyer(int maxFrameInFlight)
	{
		mMaxFrameInFlight = maxFrameInFlight;
		this->mFrameBuffers.resize(mMaxFrameInFlight);
		this->mFrameImages.resize(mMaxFrameInFlight);
		this->mFrameSamplers.resize(mMaxFrameInFlight);
		this->mFramePipelines.resize(mMaxFrameInFlight);
	}
	void DeferredDestroyer::destroyBuffer(uint64_t frame, rs_buffer_vk* buffer)
	{
		int f = frame % mMaxFrameInFlight;
		mFrameBuffers[f].push_back(buffer);
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

	void DeferredDestroyer::destroyPipeline(uint64_t frame, rs_pipeline_vk* pipeline)
	{
		int f = frame % mMaxFrameInFlight;
		mFramePipelines[f].push_back(pipeline);
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
	}

	void DeferredDestroyer::clearAll(rs_context_vk* ctx)
	{
		for (int i = 0; i < mMaxFrameInFlight; ++i) {
			endFrameDestroy(ctx, i);
		}
	}

}