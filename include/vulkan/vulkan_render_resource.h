#ifndef VULKAN_RENDER_RESOURCE_H
#define VULKAN_RENDER_RESOURCE_H

#include "render_resource.h"
#include "volk.h"
#include "vk_mem_alloc.h"

#include <array>
#include <vector>

#define VK_RS_DEF(x) typedef x x##_vk;
#define VK_CHECK(x,stmt) if(x != VK_SUCCESS){assert(0 && #x); stmt }

namespace Render::Vulkan {
	using rs_descriptor = BindingInfo;

	struct rs_queue_vk;
	
	struct rs_context_vk : rs_context{
		VkInstance instance;
		VkDevice device;
		VkPhysicalDevice physicalDevice;
		rs_queue_vk* graphicQueue;
		rs_queue_vk* computeQueue;
		rs_queue_vk* transferQueue;
		VmaAllocator allocator;
		
		//For Pipeline 
		uint32_t viewportCount = 1;
		uint32_t scissorCount = 1;
		std::vector<VkDynamicState> pipelineDyStates; // VK_DYNAMIC_STATE_VIEWPORT ,VK_DYNAMIC_STATE_SCISSOR 
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

	struct rs_shader_module_vk :rs_shader_module{
		std::vector<VkPushConstantRange>       pushConstants;
		std::vector<
			std::pair<uint16_t, std::vector<rs_descriptor>>
		> descriptorSetinfo; //set / descriptors
	};

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
		rs_vk_descriporset_layout_hash bindingHash;
	};

	struct rs_pipeline_layout_vk : rs_base {
		std::vector<std::pair<uint16_t,rs_descriptorset_layout_vk*>> setLayouts;
		std::vector<VkPushConstantRange>       pushConstants;
	};

	struct rs_pipeline_vk: rs_pipeline {
		PipelineType type{};
		rs_pipeline_layout_vk* layout;
	};
	
	VK_RS_DEF(rs_renderpass);

	VK_RS_DEF(rs_fence);

	VK_RS_DEF(rs_semaphore);

	VK_RS_DEF(rs_event);

	struct	rs_commandbuffer_vk : rs_commandbuffer {
		VkCommandPool pool;
	};

	struct	rs_swapchian_vk : rs_swapchian {
		VkSurfaceKHR surface; //Surface
		std::vector<rs_image_vk*> swapchainImgs;
	};

	VK_RS_DEF(rs_descriptorSetPool);

	VK_RS_DEF(rs_descriptorSet);

};

#endif