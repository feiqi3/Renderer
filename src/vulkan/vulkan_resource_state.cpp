#include "vulkan/vulkan_resource_state.h"
#include "vulkan/vulkan_render_resource.h"
#include "render_log.h"
#include <map>
namespace Render::Vulkan {
    VulkanStateMapping getVulkanMapping(ResourceState state) {
        switch (state) {
        case ResourceState::Common:
            // Top of pipe : NO BLOCK
            return { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED };

            // ==========================================
            // BUFFER
            // ==========================================
        case ResourceState::VertexBuffer:
            return {
                VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED
            };
        case ResourceState::IndexBuffer:
            return {
                VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                VK_ACCESS_INDEX_READ_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED
            };
        case ResourceState::UniformBuffer:
            // UBO read may happen at any time during shading
            return {
                VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_ACCESS_UNIFORM_READ_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED
            };
        case ResourceState::IndirectArgument:
            // DrawIndirect
            return {
                VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
                VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED
            };

            // ==========================================
            // Image / Buffer
            // ==========================================
        case ResourceState::ShaderResource:
            // Sampled Image
            return {
                VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };
        case ResourceState::UnorderedAccess:
            // UAV
            return {
                VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                VK_IMAGE_LAYOUT_GENERAL // Storage Image ---> General 
            };
        case ResourceState::ComputeShaderResource:
            // Sampled Image
            return {
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };
        case ResourceState::ComputeUnorderedAccess:
            // UAV
            return {
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                VK_IMAGE_LAYOUT_GENERAL // Storage Image ---> General 
            };
            // ==========================================
            // 4. Render target
            // ==========================================
        case ResourceState::RenderTarget:
            return {
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            };
        case ResourceState::DepthStencilWrite:
            // Depth stencil
            return {
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            };
        case ResourceState::DepthStencilRead:
            // Read only
            return {
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
            };

            // ==========================================
            // 5. Data transfer
            // ==========================================
        case ResourceState::TransferSrc:
            return {
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_ACCESS_TRANSFER_READ_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
            };
        case ResourceState::TransferDst:
            return {
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
            };
        case ResourceState::ResolveSrc:
            return {
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_ACCESS_TRANSFER_READ_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
            };
        case ResourceState::ResolveDst:
            return {
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
            };

            // ==========================================
            // 6. CPU
            // ==========================================
        case ResourceState::HostRead:
            return {
                VK_PIPELINE_STAGE_HOST_BIT,
                VK_ACCESS_HOST_READ_BIT,
                VK_IMAGE_LAYOUT_GENERAL
            };
        case ResourceState::HostWrite:
            return {
                VK_PIPELINE_STAGE_HOST_BIT,
                VK_ACCESS_HOST_WRITE_BIT,
                VK_IMAGE_LAYOUT_GENERAL
            };

            // ==========================================
            // ==========================================
        case ResourceState::Present:
            return {
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                0, // Present need no excilpt Access Mask Refresh
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
            };
        case ResourceState::GenericRead:
            // Default
            return {
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VK_ACCESS_MEMORY_READ_BIT,
                VK_IMAGE_LAYOUT_GENERAL
            };
        case ResourceState::General:
            return {
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
                VK_IMAGE_LAYOUT_GENERAL
            };

        default:
            Log::error("Unsupported or composite ResourceState in GetVulkanMapping!");
            return { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED };
        }
    }

    void transitionBufferState(rs_commandbuffer_vk* cb, rs_buffer_vk* buffer, ResourceState newState) {
        if (buffer->state == newState) {
            return;
        }

        VulkanStateMapping oldMap = getVulkanMapping(buffer->state);
        VulkanStateMapping newMap = getVulkanMapping(newState);

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

        vkCmdPipelineBarrier(
            (VkCommandBuffer)cb->native,
            oldMap.stageMask,
            newMap.stageMask,
            0, // dependencyFlags
            0, nullptr,
            1, &barrier,
            0, nullptr
        );

        buffer->state = newState;
    }

    void transitionImageState(
        rs_commandbuffer_vk* cb,
        rs_image_vk* image,
        ResourceState newState,
        uint32_t baseMipLevel,
        uint32_t mipLevelCount,
        uint32_t baseArrayLayer,
        uint32_t layerCount)
    {
        uint32_t actualMips = (mipLevelCount == VK_REMAINING_MIP_LEVELS) ? image->mipLevels - baseMipLevel : mipLevelCount;
        uint32_t actualLayers = (layerCount == VK_REMAINING_ARRAY_LAYERS) ? image->arrayLayers - baseArrayLayer : layerCount;

        if (image->subresourceStates.empty()) {
            image->subresourceStates.resize(image->mipLevels * image->arrayLayers, ResourceState::Common);
        }

        std::vector<VkImageMemoryBarrier> barriersToSubmit;
        VkPipelineStageFlags srcStageMaskAccumulate = 0;
        VkPipelineStageFlags dstStageMaskAccumulate = 0;

        VkImageAspectFlags aspectMask = (image->usage & ImageUsage_DepthStencilAttachment) ?
            (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT) : VK_IMAGE_ASPECT_COLOR_BIT;

        for (uint32_t layer = baseArrayLayer; layer < baseArrayLayer + actualLayers; ++layer) {
            for (uint32_t mip = baseMipLevel; mip < baseMipLevel + actualMips; ++mip) {

                uint32_t flatIndex = layer * image->mipLevels + mip;
                ResourceState oldSubState = image->subresourceStates[flatIndex];

                if (oldSubState != newState) {
                    VulkanStateMapping oldMap = getVulkanMapping(oldSubState);
                    VulkanStateMapping newMap = getVulkanMapping(newState);

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

                    srcStageMaskAccumulate |= oldMap.stageMask;
                    dstStageMaskAccumulate |= newMap.stageMask;

                    image->subresourceStates[flatIndex] = newState;
                }
            }
        }

        if (!barriersToSubmit.empty()) {
            vkCmdPipelineBarrier(
                (VkCommandBuffer)cb->native,
                srcStageMaskAccumulate, dstStageMaskAccumulate,
                0, 0, nullptr, 0, nullptr,
                static_cast<uint32_t>(barriersToSubmit.size()), barriersToSubmit.data()
            );
        }
    }

    void changeViewStateNoBarrier(rs_image_view* view, ResourceState newState)
    {
        auto baseLayer = view->viewKey.getBaseLayer();
        auto baseMipLevel = view->viewKey.getBaseMip();
        auto layerCount = view->viewKey.getLayerCount();
        auto mipCount = view->viewKey.getMipCount();
        auto mipLevels = view->image->mipLevels;
        auto image = view->image;
        for (uint32_t layer = baseLayer; layer < baseLayer + layerCount; ++layer) {
            for (uint32_t mip = baseMipLevel; mip < baseMipLevel + mipCount; ++mip) {
                uint32_t flatIndex = layer * mipLevels + mip;
                image->subresourceStates[flatIndex] = newState;
            }
        }
    }

}