#ifndef VULKAN_RESOURCE_STATE_H_
#define VULKAN_RESOURCE_STATE_H_
#include "vulkan_render_resource.h"
namespace Render::Vulkan {

    struct VulkanStateMapping {
        VkPipelineStageFlags stageMask;
        VkAccessFlags accessMask;      
        VkImageLayout imageLayout;     
    };
    VulkanStateMapping getVulkanMapping(ResourceState state);
    void transitionBufferState(rs_commandbuffer_vk* cb, rs_buffer_vk* buffer, ResourceState newState);

    void transitionImageState(
        rs_commandbuffer_vk* cb,
        rs_image_vk* image,
        ResourceState newState,
        uint32_t baseMipLevel = 0,
        uint32_t mipLevelCount = VK_REMAINING_MIP_LEVELS,
        uint32_t baseArrayLayer = 0,
        uint32_t layerCount = VK_REMAINING_ARRAY_LAYERS
    );

    
    void changeViewStateNoBarrier(
        rs_image_view* view,ResourceState newState
    );
}

#endif