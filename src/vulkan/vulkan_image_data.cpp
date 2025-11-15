#include "vulkan/vulkan_image_data.h"
#include "vulkan/vulkan_render_function.h"
namespace Render::Vulkan {
    ImageDataManager::ImageDataManager(uint32_t maxFrameInFlight)
    {
        mMaxFrameInFlight = maxFrameInFlight;
    }
    void ImageDataManager::beginRenderFrame(uint64_t targetFrame, rs_context_vk* ctx)
    {
        CommandBufferDesc desc{ .queueType = QueueType_Graphics,.transient = true };
        if (!this->fence) {
            fence = createRsFence(ctx);
            resetRsFence(ctx, fence);
        }
    }
    void ImageDataManager::updateImageData(uint32_t fif, rs_context_vk* ctx, rs_image_vk* image, void* data,size_t byteSize, int x, int y, int z, int width, int height, int depth, int layeroff, int layerSize, int mip)
    {
        CommandBufferDesc desc{.queueType = QueueType_Graphics,.transient = true };

        auto cmd = createRsCommand(ctx, desc);
        cmd->hasCommands = true;
        cmdBeginRecord(cmd);
        cmdUpdateImage(cmd, ctx, image, data, byteSize, x, y, z, width, height, depth, mip, layeroff,layerSize);
        cmdEndRecord(cmd);
        cmdSubmitCmdBuffer(ctx, cmd, QueueType_Graphics, {}, {}, (Vulkan::rs_fence_vk*)fence);
        if (fence) {
            waitForRsFence(ctx, fence, -1, ctx->RenderFrameFif);
            resetRsFence(ctx, fence);
        }
      
    }

    void ImageDataManager::updateBufferData(uint32_t fif, rs_context_vk* ctx, rs_buffer_vk* buffer, void* data, size_t byteSize, size_t offsetDst)
    {
        CommandBufferDesc desc{ .queueType = QueueType_Graphics,.transient = true };

        auto cmd = createRsCommand(ctx, desc);
        cmd->hasCommands = true;
        cmdBeginRecord(cmd);
        Vulkan::cmdUpdateBufferData(cmd, ctx, buffer, data, byteSize, offsetDst);
        cmdEndRecord(cmd);
        Vulkan::cmdSubmitCmdBuffer(ctx, cmd, QueueType_Graphics, {}, {}, (Vulkan::rs_fence_vk*)fence);
        if (fence) {
            waitForRsFence(ctx, fence, -1, ctx->RenderFrameFif);
            resetRsFence(ctx, fence);
        }
    }

    void ImageDataManager::cmdUpdateImageData(uint32_t fif, rs_context_vk* ctx, rs_commandbuffer_vk* cmd, rs_image_vk* image, void* data, size_t byteSize, int x, int y, int z, int width, int height, int depth, int layeroff, int layerSize, int mip)
    {
        auto pendingBuffer = createStageBufferTemp(ctx, byteSize);
        auto dst = mapRsBuffer(ctx, pendingBuffer);
        memcpy(dst, data, byteSize);
        VkMappedMemoryRange range{};
        range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.memory = getDeviceMemory(pendingBuffer->allocation);
        range.offset = getOffsetAllocation(pendingBuffer->allocation);
        range.size = byteSize;
        vkFlushMappedMemoryRanges(ctx->device, 1, &range);
        PendingUpdateInfo info{};
        info.buffer = pendingBuffer;
        info.datasize = byteSize;
        info.x = x;
        info.y = y;
        info.z = z;
        info.width = width;
        info.height = height;
        info.depth = depth;
        info.mip = mip;
        info.layeroff = layeroff;
        info.layersize = layerSize;
        recordUpdateCmd(ctx, cmd, info);
    }

    void ImageDataManager::cmdUpdateBufferData(uint32_t fif, rs_context_vk* ctx, rs_commandbuffer_vk* cmd, rs_buffer_vk* buffer, void* data, size_t byteSize, size_t offsetDst)
    {
        auto pendingBuffer = createStageBufferTemp(ctx, byteSize);
        auto dst = mapRsBuffer(ctx, pendingBuffer);
        memcpy(dst, data, byteSize);
        PendingBufferUpdateInfo info{};
        info.pendingBuffer = pendingBuffer;
        info.buffer = buffer;
        info.size = byteSize;
        info.offsetDst = offsetDst;
        VkMappedMemoryRange range{};
        range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.memory = getDeviceMemory(pendingBuffer->allocation);
        range.offset = getOffsetAllocation(pendingBuffer->allocation);
        range.size = byteSize;
        vkFlushMappedMemoryRanges(ctx->device, 1, &range);
        recordUpdateCmd(ctx, cmd, info);
    }

    void ImageDataManager::clearAll(rs_context_vk* ctx) {
        destroyRsFence(ctx, fence);
        fence = nullptr;
    }

    ImageDataManager::~ImageDataManager()
    {
    }
    void ImageDataManager::recordUpdateCmd(rs_context_vk* ctx, rs_commandbuffer_vk* cmd, PendingUpdateInfo& pendingInfo)
    {
        cmdUpdateImage(cmd, ctx, pendingInfo.target, pendingInfo.buffer, pendingInfo.x, pendingInfo.y, pendingInfo.z, pendingInfo.width, pendingInfo.height, pendingInfo.depth, pendingInfo.mip, pendingInfo.layeroff,pendingInfo.layersize);
        destroyRsBuffer(ctx, pendingInfo.buffer);
    }
    void ImageDataManager::recordUpdateCmd(rs_context_vk* ctx, rs_commandbuffer_vk* cmd, PendingBufferUpdateInfo& pendingInfo)
    {
        cmdCopyBufferToBuffer(cmd, ctx, pendingInfo.pendingBuffer, pendingInfo.buffer, pendingInfo.size, 0, pendingInfo.offsetDst);
        destroyRsBuffer(ctx, pendingInfo.pendingBuffer);
    }
}