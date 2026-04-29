#ifndef VULKAN_DEFERRED_DESTROY_H
#define VULKAN_DEFERRED_DESTROY_H
#include "vulkan_render_resource.h"
#include <vector>
namespace Render::Vulkan {
	struct rs_buffer_vk;
	struct rs_image_vk;
	struct rs_sampler_vk;
	struct rs_graphic_pipeline_vk;
	struct rs_renderpass_vk;
	struct rs_context_vk;
	struct rs_rendertarget_vk;
	struct rs_semaphore_vk;
	struct rs_fence_vk;
	struct rs_drawdata_vk;
}

namespace Render::Vulkan {

	class DeferredDestroyer {
	public:
		DeferredDestroyer(int maxFrameInFlight);
		void destroyBindlessData(uint64_t frame, rs_bindless_data_vk* data);
		void destroyDrawData(uint64_t frame, rs_drawdata_vk* buffer);
		void destroyBuffer(uint64_t frame, rs_buffer_vk* buffer);
		void destroyRenderTarget(uint64_t frame, rs_rendertarget_vk* rt);
		void destroyImage(uint64_t frame, rs_image_vk* image);
		void destroySampler(uint64_t frame, rs_sampler_vk* sampler);
		void destroyPipeline(uint64_t frame, rs_pipeline* pipeline);
		void destroyRenderPass(uint64_t frame, rs_renderpass_vk* renderpass);
		void destroySemaphores(rs_semaphore_vk* semaphores);
		void destroyFences(rs_fence_vk* fences);
		void endFrameDestroy(rs_context_vk* ctx ,uint64_t frame);
		void clearAll(rs_context_vk* ctx);
	private:
		std::vector<std::vector<rs_rendertarget_vk*>> mFrameRendertargets;

		std::vector<std::vector<rs_buffer_vk*>> mFrameBuffers;

		std::vector<std::vector<rs_image_vk*>> mFrameImages;

		std::vector<std::vector<rs_sampler_vk*>> mFrameSamplers;

		std::vector<std::vector<rs_pipeline*>> mFramePipelines;

		std::vector<std::vector<rs_renderpass_vk*>> mFrameRenderPasses;

		std::vector<std::vector<rs_drawdata_vk*>> mFrameDrawDatas;

		std::vector<std::vector<rs_bindless_data_vk*>> mFrameBindlessDatas;

		std::vector<std::vector<void*>> mFrameSemaphores;

		std::vector<std::vector<void*>> mFrameFences;
		int mMaxFrameInFlight = 3;
	};

}

#endif // !VULKAN_DEFERRED_DESTROY_H
