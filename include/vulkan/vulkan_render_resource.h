#include "render_resource.h"
#include "volk.h"
#include "vk_mem_alloc.h"

#include <array>
#include <vector>

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
		class DescriptorSetManager* descriptorSetMgr = 0;
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

	struct rs_descriptorset_layout_vk : rs_base {
	

		inline void accquir() {
			ref++;
		}
		inline void release() {
			ref--;
			assert(ref >= 0 && "Wrong ref count");
		}

		std::atomic_uint32_t ref = 0;
	
	};

	struct rs_pipeline_layout_vk : rs_base {
		std::vector<std::pair<uint16_t,rs_descriptorset_layout_vk*>> setLayouts;
		std::vector<VkPushConstantRange>       pushConstants;

	};

	struct rs_pipeline_vk {
		rs_pipeline_layout_vk* layout;
		PipelineType type{};
	};

	struct rs_vk_descriporset_layout_hash {
		std::array<uint64_t, 4> data{};
	};


	
	VK_RS_DEF(rs_renderpass);

	VK_RS_DEF(rs_fence);

	VK_RS_DEF(rs_semaphore);

	VK_RS_DEF(rs_event);

	struct	rs_commandbuffer_vk : rs_commandbuffer {
		VkCommandPool pool;
	};

	VK_RS_DEF(rs_descriptorSetPool);

	VK_RS_DEF(rs_descriptorSet);

};