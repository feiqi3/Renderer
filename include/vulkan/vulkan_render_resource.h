#include "render_resource.h"
#include "volk.h"
#include "vk_mem_alloc.h"

#define VK_RS_DEF(x) typedef x x##_vk;

namespace Render::Vulkan {
	struct rs_queue_vk;
	
	struct rs_context_vk : rs_context{
		VkInstance instance;
		VkDevice device;
		VkPhysicalDevice physicalDevice;
		rs_queue_vk* graphicQueue;
		rs_queue_vk* computeQueue;
		rs_queue_vk* transferQueue;
		VmaAllocator allocator;
	};

	struct rs_queue_vk : rs_queue{
		VkQueue queue;
		uint32_t familyIndex;
	};

	struct rs_buffer_vk : rs_buffer {
		VmaAllocation allocation;
	};

	struct rs_image_vk : rs_image {
		VmaAllocation allocation;
		VkImageView view;
	};

	VK_RS_DEF(rs_shader_module);

	VK_RS_DEF(rs_sampler);

	VK_RS_DEF(rs_pipeline);

	VK_RS_DEF(rs_pipelineLayout);
	
	VK_RS_DEF(rs_renderpass);

	VK_RS_DEF(rs_fence);

	VK_RS_DEF(rs_semaphore);

	VK_RS_DEF(rs_event);

	struct	rs_commandbuffer_vk : rs_commandbuffer {
		VkCommandPool pool;
	};

	VK_RS_DEF(rs_descriptorSetPool);

	VK_RS_DEF(rs_descriptorSet);

	VK_RS_DEF(rs_descriptorSetLayout);

};