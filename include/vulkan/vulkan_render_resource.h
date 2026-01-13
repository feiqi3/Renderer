#ifndef VULKAN_RENDER_RESOURCE_H
#define VULKAN_RENDER_RESOURCE_H

#include "render_resource.h"
#define VK_NO_PROTOTYPES
#include "volk.h"
#include "vk_mem_alloc.h"
#include "vulkan_deferred_destroy.h"
#include  <utility>
#include <array>
#include <vector>
#include <mutex>
#define VK_RS_DEF(x) typedef x x##_vk;
#define VK_CHECK(x,stmt) if(x != VK_SUCCESS){assert(0 && #x);Log::error(#x); stmt }

namespace Render::Vulkan {
	struct rs_queue_vk;
	struct rs_swapchain_vk;
	using VkObjHandle = void*;
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
		bool dynamicWireFrameStateSupported = false;
		//

		class DescriptorSetManager* descriptorSetMgr = 0;
		class CommandBufferManager* cmdBufferMgr = 0;
		class DeferredDestroyer* destroyer = 0;
		class ImageDataManager* imageDataMgr = 0;

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
	};

	struct frame_buffer_cache {
		uint64_t renderPassHash;
		uint64_t lastUsedFrame;
		VkObjHandle frameBuffer;
	};

	struct rs_rendertarget_vk : rs_rendertarget {
		uint64_t rtPassHash;
		std::mutex mMutex;
	};

	struct rs_pipeline_vk: rs_pipeline {
		VkObjHandle wireFramePipeline = nullptr;
		PipelineType type{};
		rs_pipeline_layout_vk* layout = nullptr;
	};

	struct rs_renderpass_vk :rs_renderpass {
		//What does pass hash contains? 1. image format, 2. sample count 
		uint64_t passHash = 0;
	};

	struct rs_fence_vk : rs_fence {
		int cnt = 0;
	};

	struct rs_semaphore_vk : rs_semaphore {
		int cnt = 0;
	};

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

	struct rs_descriptorSet_vk :rs_descriptorSet{
		rs_descriptorset_layout_vk* layout;
		struct DescriptorPoolBlock* pool;
	};

	struct rs_validation_vk :rs_base {
		
	};

	struct vk_binding_pos {
		int16_t setIdx = -1;
		int16_t bindingIdx = -1;
	};

	inline vk_binding_pos toVkBindingPos(rs_binding_pos pos) {
		const uint32_t HIGH_SETID_MASK = 0xFFFF0000;
		const uint32_t LOW_BINDINGID_MASK = 0x0000FFFF;
		vk_binding_pos bindingPos{
			.setIdx = int16_t((pos & HIGH_SETID_MASK) >> 16),
			.bindingIdx = int16_t(pos & LOW_BINDINGID_MASK)
		};
		return bindingPos;
	}

	inline rs_binding_pos toRsBindingPos(vk_binding_pos bindingPos) {
		uint32_t bindingPosRs = (uint16_t)bindingPos.setIdx;
		bindingPosRs = bindingPosRs << 16;
		bindingPosRs = bindingPosRs | (uint16_t)bindingPos.bindingIdx;
		return bindingPosRs;
	}

	struct rs_drawdata_vk : rs_drawdata {
		std::vector <			//For each Frame
			std::vector<		//Each Descriptorset
				std::pair<uint16_t, rs_descriptorSet_vk*>
			>
		> DescriptorSets; //per frame in flight, per descriptor
	};

};

#endif