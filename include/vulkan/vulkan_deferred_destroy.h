#ifndef VULKAN_DEFERRED_DESTROY_H
#include "vulkan_render_resource.h"
namespace Render::Vulkan {

	class DeferredDestroyer {
	public:
		DeferredDestroyer(int maxFrameInFlight);

		void destroyBuffer(uint64_t frame, rs_buffer_vk* buffer);
		void destroyImage(uint64_t frame, rs_image_vk* image);
		void destroySampler(uint64_t frame, rs_sampler_vk* sampler);
		void destroyPipeline(uint64_t frame, rs_pipeline_vk* pipeline);

		void endFrameDestroy(rs_context_vk* ctx ,uint64_t frame);
	private:
		std::vector<
			std::vector<rs_buffer_vk*>
		> mFrameBuffers;

		std::vector<
			std::vector<rs_image_vk*>
		> mFrameImages;

		std::vector<
			std::vector<rs_sampler_vk*>
		> mFrameSamplers;

		std::vector<
			std::vector<rs_pipeline_vk*>
		> mFramePipelines;

		int mMaxFrameInFlight = 3;
	};

}

#endif // !VULKAN_DEFERRED_DESTROY_H
