#ifndef VULKAN_RESOURCE_STATE_H_
#define VULKAN_RESOURCE_STATE_H_
#include "vulkan_render_resource.h"
#include "vulkan_global_def.h"
#include <map>
inline bool AggressiveBatchBarrier = false;
inline bool DisableSync2 = false;;

namespace Render::Vulkan {

    struct VulkanStateMapping {
        VkPipelineStageFlags stageMask;
        VkAccessFlags accessMask;      
        VkImageLayout imageLayout;     
    };
    VulkanStateMapping getVulkanMapping(ResourceState state);
    void transitionBufferState(rs_commandbuffer_vk* cb, rs_buffer_vk* buffer, ResourceState newState);
    void transitionBufferStateBatch(rs_commandbuffer_vk* cb, auto buffersItor,auto buffersItorEnd, ResourceState newState);
    void transitionImageState(
        rs_commandbuffer_vk* cb,
        rs_image_vk* image,
        ResourceState newState,
        uint32_t baseMipLevel = 0,
        uint32_t mipLevelCount = VK_REMAINING_MIP_LEVELS,
        uint32_t baseArrayLayer = 0,
        uint32_t layerCount = VK_REMAINING_ARRAY_LAYERS
    );

    void transitionImageStateBatch(
        rs_commandbuffer_vk* cb,
        auto itorBegin,
        auto itorEnd,
        ResourceState newState
    );
    
    void changeViewStateNoBarrier(
        rs_image_view* view,ResourceState newState
    );


    ///////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////

        /*
    From ZEUX: https://zeux.io/2020/02/27/writing-an-efficient-vulkan-renderer/
    When generating commands for each individual barrier,
    the driver only has a local view of the barrier and is unaware of past or future barriers.
    Because of this, the first important rule is that barriers need to be batched as aggressively as possible
    So we have three version here
    */
    inline void transitionBufferStateBatch(rs_commandbuffer_vk* cb, auto buffersItor, auto buffersItorEnd, ResourceState newState)
    {
        bool Sync2ExtEnabled = (DisableSync2 == false) && (Synchronize2Enable);
        if (Sync2ExtEnabled) {
            std::vector<VkBufferMemoryBarrier2> barriersToSubmit;
            barriersToSubmit.reserve(std::distance(buffersItor, buffersItorEnd));

            VulkanStateMapping newMap = getVulkanMapping(newState);

            for (auto it = buffersItor; it != buffersItorEnd; ++it)
            {
                rs_buffer_vk* bufferVk = (rs_buffer_vk*)(*it);

                if (bufferVk->state == newState) {
                    continue;
                }

                VulkanStateMapping oldMap = getVulkanMapping(bufferVk->state);

                VkBufferMemoryBarrier2 barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
                barrier.pNext = nullptr;

                barrier.srcStageMask = oldMap.stageMask;
                barrier.srcAccessMask = oldMap.accessMask;
                barrier.dstStageMask = newMap.stageMask;
                barrier.dstAccessMask = newMap.accessMask;

                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.buffer = (VkBuffer)bufferVk->native;
                barrier.offset = 0;
                barrier.size = VK_WHOLE_SIZE;

                barriersToSubmit.push_back(barrier);

                bufferVk->state = newState;
                assert(bufferVk->pendingState == bufferVk->state);
            }

            if (barriersToSubmit.empty()) {
                return;
            }

            VkDependencyInfo dependencyInfo{};
            dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            dependencyInfo.pNext = nullptr;
            dependencyInfo.dependencyFlags = 0;
            dependencyInfo.bufferMemoryBarrierCount = (uint32_t)barriersToSubmit.size();
            dependencyInfo.pBufferMemoryBarriers = barriersToSubmit.data();

            vkCmdPipelineBarrier2((VkCommandBuffer)cb->native, &dependencyInfo);
            return;
        }

        if (AggressiveBatchBarrier) {
            std::vector<VkBufferMemoryBarrier> barriersToSubmit;
            barriersToSubmit.reserve(std::distance(buffersItor, buffersItorEnd));

            VkPipelineStageFlags globalSrcStageMask = 0;
            VkPipelineStageFlags globalDstStageMask = 0;
            VulkanStateMapping newMap = getVulkanMapping(newState);

            for (auto it = buffersItor; it != buffersItorEnd; ++it)
            {
                rs_buffer_vk* bufferVk = (rs_buffer_vk*)(*it);

                if (bufferVk->state == newState) {
                    continue;
                }

                VulkanStateMapping oldMap = getVulkanMapping(bufferVk->state);

                globalSrcStageMask |= oldMap.stageMask;
                globalDstStageMask |= newMap.stageMask;

                VkBufferMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                barrier.pNext = nullptr;
                barrier.srcAccessMask = oldMap.accessMask;
                barrier.dstAccessMask = newMap.accessMask;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.buffer = (VkBuffer)bufferVk->native;
                barrier.offset = 0;
                barrier.size = VK_WHOLE_SIZE;

                barriersToSubmit.push_back(barrier);

                bufferVk->state = newState;
                assert(bufferVk->pendingState == bufferVk->state);
            }

            if (barriersToSubmit.empty()) {
                return;
            }

            vkCmdPipelineBarrier(
                (VkCommandBuffer)cb->native,
                globalSrcStageMask,
                globalDstStageMask,
                0, // dependencyFlags
                0, nullptr,
                (uint32_t)barriersToSubmit.size(), barriersToSubmit.data(),
                0, nullptr
            );
        }
        else {
            std::map<ResourceState, std::vector<rs_buffer_vk*>> buffersMap;
            for (auto it = buffersItor; it != buffersItorEnd; ++it)
            {
                rs_buffer_vk* buffer = (rs_buffer_vk*)(*it);
                if (buffer->state != newState) {
                    buffersMap[buffer->state].push_back(buffer);
                }
            }

            VulkanStateMapping newMap = getVulkanMapping(newState);

            for (auto& [oldState, buffers] : buffersMap)
            {
                if (buffers.empty()) continue;

                VulkanStateMapping oldMap = getVulkanMapping(oldState);
                std::vector<VkBufferMemoryBarrier> barriersToSubmit;
                barriersToSubmit.reserve(buffers.size());

                for (auto buffer : buffers) {
                    VkBufferMemoryBarrier barrier{};
                    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                    barrier.pNext = nullptr;
                    barrier.srcAccessMask = oldMap.accessMask;
                    barrier.dstAccessMask = newMap.accessMask;
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.buffer = (VkBuffer)buffer->native;
                    barrier.offset = 0;
                    barrier.size = VK_WHOLE_SIZE;

                    barriersToSubmit.push_back(barrier);

                    buffer->state = newState;
                    assert(bufferVk->pendingState == bufferVk->state);
                }

                vkCmdPipelineBarrier(
                    (VkCommandBuffer)cb->native,
                    oldMap.stageMask,
                    newMap.stageMask,
                    0, // dependencyFlags
                    0, nullptr,
                    (uint32_t)barriersToSubmit.size(), barriersToSubmit.data(),
                    0, nullptr
                );
            }
        }
    }


    void transitionImageStateBatch(
        rs_commandbuffer_vk* cb,
        auto itorBegin,
        auto itorEnd,
        ResourceState newState)
    {
        if (itorBegin == itorEnd) return;

        VulkanStateMapping newMap = getVulkanMapping(newState);

        bool Sync2ExtEnabled = (DisableSync2 == false) && (Synchronize2Enable);
        if (Sync2ExtEnabled)
        {
            std::vector<VkImageMemoryBarrier2> barriersToSubmit;

            for (auto it = itorBegin; it != itorEnd; ++it)
            {
                rs_image_view* view = (rs_image_view*)(*it);
                rs_image_vk* image = (rs_image_vk*)view->image;
                auto& key = view->viewKey;

                uint32_t actualMips = (key.getMipCount() == VK_REMAINING_MIP_LEVELS) ? image->mipLevels - key.getBaseMip() : key.getMipCount();
                uint32_t actualLayers = (key.getLayerCount() == VK_REMAINING_ARRAY_LAYERS) ? image->arrayLayers - key.getBaseLayer() : key.getLayerCount();

                if (image->subresourceStates.empty()) {
                    image->subresourceStates.resize(image->mipLevels * image->arrayLayers, ResourceState::Common);
                }

                VkImageAspectFlags aspectMask = (image->usage & ImageUsage_DepthStencilAttachment) ?
                    (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT) : VK_IMAGE_ASPECT_COLOR_BIT;

                for (uint32_t layer = key.getBaseLayer(); layer < key.getBaseLayer() + actualLayers; ++layer) {
                    for (uint32_t mip = key.getBaseMip(); mip < key.getBaseMip() + actualMips; ++mip) {

                        uint32_t flatIndex = layer * image->mipLevels + mip;
                        ResourceState oldSubState = image->subresourceStates[flatIndex];

                        if (oldSubState != newState) {
                            VulkanStateMapping oldMap = getVulkanMapping(oldSubState);

                            VkImageMemoryBarrier2 barrier{};
                            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                            barrier.pNext = nullptr;
                            barrier.oldLayout = oldMap.imageLayout;
                            barrier.newLayout = newMap.imageLayout;
                            barrier.srcStageMask = oldMap.stageMask;
                            barrier.srcAccessMask = oldMap.accessMask;
                            barrier.dstStageMask = newMap.stageMask;
                            barrier.dstAccessMask = newMap.accessMask;
                            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                            barrier.image = (VkImage)image->native;

                            barrier.subresourceRange.aspectMask = aspectMask;
                            barrier.subresourceRange.baseMipLevel = mip;
                            barrier.subresourceRange.levelCount = 1;
                            barrier.subresourceRange.baseArrayLayer = layer;
                            barrier.subresourceRange.layerCount = 1;

                            barriersToSubmit.push_back(barrier);
                            image->subresourceStates[flatIndex] = newState;
                            assert(image->subresourceStates[flatIndex] == image->subresourcePendingStates[flatIndex]);
                        }
                    }
                }
            }

            if (!barriersToSubmit.empty()) {
                VkDependencyInfo dependencyInfo{};
                dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
                dependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(barriersToSubmit.size());
                dependencyInfo.pImageMemoryBarriers = barriersToSubmit.data();
                vkCmdPipelineBarrier2((VkCommandBuffer)cb->native, &dependencyInfo);
            }
        }
        else if (AggressiveBatchBarrier)
        {
            std::vector<VkImageMemoryBarrier> barriersToSubmit;
            VkPipelineStageFlags globalSrcStageMask = 0;
            VkPipelineStageFlags globalDstStageMask = 0;

            for (auto it = itorBegin; it != itorEnd; ++it)
            {
                rs_image_view* view = (rs_image_view*)(*it);
                rs_image_vk* image = (rs_image_vk*)view->image;
                auto& key = view->viewKey;

                uint32_t actualMips = (key.getMipCount() == VK_REMAINING_MIP_LEVELS) ? image->mipLevels - key.getBaseMip() : key.getMipCount();
                uint32_t actualLayers = (key.getLayerCount() == VK_REMAINING_ARRAY_LAYERS) ? image->arrayLayers - key.getBaseLayer() : key.getLayerCount();

                if (image->subresourceStates.empty()) {
                    image->subresourceStates.resize(image->mipLevels * image->arrayLayers, ResourceState::Common);
                }

                VkImageAspectFlags aspectMask = (image->usage & ImageUsage_DepthStencilAttachment) ?
                    (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT) : VK_IMAGE_ASPECT_COLOR_BIT;

                for (uint32_t layer = key.getBaseLayer(); layer < key.getBaseLayer() + actualLayers; ++layer) {
                    for (uint32_t mip = key.getBaseMip(); mip < key.getBaseMip() + actualMips; ++mip) {

                        uint32_t flatIndex = layer * image->mipLevels + mip;
                        ResourceState oldSubState = image->subresourceStates[flatIndex];

                        if (oldSubState != newState) {
                            VulkanStateMapping oldMap = getVulkanMapping(oldSubState);

                            globalSrcStageMask |= oldMap.stageMask;
                            globalDstStageMask |= newMap.stageMask;

                            VkImageMemoryBarrier barrier{};
                            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                            barrier.oldLayout = oldMap.imageLayout;
                            barrier.newLayout = newMap.imageLayout;
                            barrier.srcAccessMask = oldMap.accessMask;
                            barrier.dstAccessMask = newMap.accessMask;
                            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                            barrier.image = (VkImage)image->native;

                            barrier.subresourceRange.aspectMask = aspectMask;
                            barrier.subresourceRange.baseMipLevel = mip;
                            barrier.subresourceRange.levelCount = 1;
                            barrier.subresourceRange.baseArrayLayer = layer;
                            barrier.subresourceRange.layerCount = 1;

                            barriersToSubmit.push_back(barrier);
                            image->subresourceStates[flatIndex] = newState;
                            assert(image->subresourceStates[flatIndex] == image->subresourcePendingStates[flatIndex]);
                        }
                    }
                }
            }

            if (!barriersToSubmit.empty()) {
                vkCmdPipelineBarrier(
                    (VkCommandBuffer)cb->native,
                    globalSrcStageMask, globalDstStageMask,
                    0, 0, nullptr, 0, nullptr,
                    static_cast<uint32_t>(barriersToSubmit.size()), barriersToSubmit.data()
                );
            }
        }
        else
        {
            std::map<ResourceState, std::vector<VkImageMemoryBarrier>> barriersByOldState;

            for (auto it = itorBegin; it != itorEnd; ++it)
            {
                rs_image_view* view = (rs_image_view*)(*it);
                rs_image_vk* image = (rs_image_vk*)view->image;
                auto& key = view->viewKey;

                uint32_t actualMips = (key.getMipCount() == VK_REMAINING_MIP_LEVELS) ? image->mipLevels - key.getBaseMip() : key.getMipCount();
                uint32_t actualLayers = (key.getLayerCount() == VK_REMAINING_ARRAY_LAYERS) ? image->arrayLayers - key.getBaseLayer() : key.getLayerCount();

                if (image->subresourceStates.empty()) {
                    image->subresourceStates.resize(image->mipLevels * image->arrayLayers, ResourceState::Common);
                }

                VkImageAspectFlags aspectMask = (image->usage & ImageUsage_DepthStencilAttachment) ?
                    (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT) : VK_IMAGE_ASPECT_COLOR_BIT;

                for (uint32_t layer = key.getBaseLayer(); layer < key.getBaseLayer() + actualLayers; ++layer) {
                    for (uint32_t mip = key.getBaseMip(); mip < key.getBaseMip() + actualMips; ++mip) {

                        uint32_t flatIndex = layer * image->mipLevels + mip;
                        ResourceState oldSubState = image->subresourceStates[flatIndex];

                        if (oldSubState != newState) {
                            VulkanStateMapping oldMap = getVulkanMapping(oldSubState);

                            VkImageMemoryBarrier barrier{};
                            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                            barrier.oldLayout = oldMap.imageLayout;
                            barrier.newLayout = newMap.imageLayout;
                            barrier.srcAccessMask = oldMap.accessMask;
                            barrier.dstAccessMask = newMap.accessMask;
                            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                            barrier.image = (VkImage)image->native;

                            barrier.subresourceRange.aspectMask = aspectMask;
                            barrier.subresourceRange.baseMipLevel = mip;
                            barrier.subresourceRange.levelCount = 1;
                            barrier.subresourceRange.baseArrayLayer = layer;
                            barrier.subresourceRange.layerCount = 1;

                            barriersByOldState[oldSubState].push_back(barrier);
                            image->subresourceStates[flatIndex] = newState;
                            assert(image->subresourceStates[flatIndex] == image->subresourcePendingStates[flatIndex]);
                        }
                    }
                }
            }
            for (auto& [oldState, barriers] : barriersByOldState) {
                VulkanStateMapping oldMap = getVulkanMapping(oldState);
                vkCmdPipelineBarrier(
                    (VkCommandBuffer)cb->native,
                    oldMap.stageMask, newMap.stageMask,
                    0, 0, nullptr, 0, nullptr,
                    static_cast<uint32_t>(barriers.size()), barriers.data()
                );
            }
        }
    }

}

#endif