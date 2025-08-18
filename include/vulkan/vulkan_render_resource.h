#ifndef VULKAN_RENDER_RESOURCE_H
#define VULKAN_RENDER_RESOURCE_H

#include "render_resource.h"
#define VK_NO_PROTOTYPES
#include "volk.h"
#include "vk_mem_alloc.h"
#include "vulkan_deferred_destroy.h"
#include <array>
#include <vector>

#define VK_RS_DEF(x) typedef x x##_vk;
#define VK_CHECK(x,stmt) if(x != VK_SUCCESS){assert(0 && #x); stmt }

namespace Render::Vulkan {
	struct rs_queue_vk;
	struct rs_swapchain_vk;

	struct rs_context_vk : rs_context{
		VkInstance instance;
		VkDevice device;
		VkPhysicalDevice physicalDevice;
		VkPhysicalDeviceProperties physicalDeviceProperties;
		rs_queue_vk* presentQueue;
		rs_queue_vk* graphicQueue;
		rs_queue_vk* computeQueue;
		rs_queue_vk* transferQueue;
		VmaAllocator allocator;
		
		rs_swapchain_vk* swapchain = nullptr;
		uint32_t maxSwapChainImages;
		uint32_t currentSwapchainImage = 0;
		VkDebugUtilsMessengerEXT validationObject;

		bool mIsValidationLayerEnabled = false;


		//For Pipeline 
		uint32_t viewportCount = 1;
		uint32_t scissorCount = 1;

		class DescriptorSetManager* descriptorSetMgr = 0;
		class CommandBufferManager* cmdBufferMgr = 0;
		class DeferredDestroyer* destroyer = 0;

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
		VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	};

	struct rs_shader_module_vk :rs_shader_module{
		std::vector<uint32_t> spirvCode;
	};

	struct rs_sampler_vk : rs_sampler {};

	struct rs_pipeline_layout_vk : rs_base {
		std::vector<std::pair<uint16_t,struct rs_descriptorset_layout_vk*>> setLayouts;
		std::vector<VkPushConstantRange>       pushConstants;
	};

	struct rs_rendertarget_vk : rs_rendertarget {};

	struct rs_pipeline_vk: rs_pipeline {
		PipelineType type{};
		rs_pipeline_layout_vk* layout;
	};
	
	struct rs_renderpass_vk :rs_renderpass {
		VkFramebuffer frameBuffer;
	};

	VK_RS_DEF(rs_fence);

	VK_RS_DEF(rs_semaphore);

	VK_RS_DEF(rs_event);

	struct rs_commandpool_vk : rs_base {
		rs_queue_vk* queue;
	};

	struct	rs_commandbuffer_vk : rs_commandbuffer {
		rs_commandpool_vk* pool;
	};

	struct	rs_swapchain_vk : rs_swapchain {
		VkSurfaceKHR surface; //Surface
		std::vector<rs_image_vk*> swapchainImgs;
	};

	VK_RS_DEF(rs_descriptorSetPool);

	struct rs_descriptorSet_vk :rs_descriptorSet{
		rs_descriptorset_layout_vk* layout;
		struct DescriptorPoolBlock* pool;
	};

	struct rs_validation_vk :rs_base {
		
	};

};

#endif