#include "vulkan/vulkan_image_data.h"
#include "vulkan/vulkan_render_function.h"
namespace Render::Vulkan {
    ImageDataManager::ImageDataManager(uint32_t maxFrameInFlight)
    {
        mMaxFrameInFlight = maxFrameInFlight;
    }
    void ImageDataManager::beginRenderFrame(uint32_t fif, rs_context_vk* ctx)
    {
        CommandBufferDesc desc{ .queueType = QueueType_Graphics,.transient = true };

        auto cmd = createRsCommand(ctx, desc);
        auto& pendingInfo = mPendingDataInfo[fif];
        for (auto&& info : pendingInfo) {
            recordUpdateCmd(ctx, cmd, info);
        }
        cmdSubmitCmdBuffer(ctx, cmd, QueueType_Graphics, {}, {}, 0);
    }
    void ImageDataManager::updateImageData(uint32_t fif, rs_context_vk* ctx, rs_image_vk* image, void* data,size_t byteSize, int x, int y, int z, int width, int height, int depth, int layeroff, int layerSize, int mip, bool imm)
    {
        if (imm) {
            CommandBufferDesc desc{.queueType = QueueType_Graphics,.transient = true };

            auto cmd = createRsCommand(ctx, desc);
            cmdUpdateImage(cmd, ctx, image, data, byteSize, x, y, z, width, height, depth, mip, layeroff,layerSize);
            cmdSubmitCmdBuffer(ctx, cmd, QueueType_Graphics, {}, {}, 0);
        }
        else {
            auto pendingPtr = malloc(byteSize);
            auto pendingBuffer = createStageBufferTemp(ctx, byteSize);
            auto dst = mapRsBuffer(ctx, pendingBuffer);
            memcpy(dst, data, byteSize);
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
            mPendingDataInfo[fif].push_back(info);
        }
    }

    void ImageDataManager::clearAll(rs_context_vk* ctx) {
        for (auto&& pendings : mPendingDataInfo) {
            for (auto&& info : pendings) {
                destroyRsBuffer(ctx,info.buffer);
            }
        }
    }

    ImageDataManager::~ImageDataManager()
    {

    }
    void ImageDataManager::recordUpdateCmd(rs_context_vk* ctx, rs_commandbuffer_vk* cmd, PendingUpdateInfo& pendingInfo)
    {
        cmdUpdateImage(cmd, ctx, pendingInfo.target, pendingInfo.buffer, pendingInfo.x, pendingInfo.y, pendingInfo.z, pendingInfo.width, pendingInfo.height, pendingInfo.depth, pendingInfo.mip, pendingInfo.layeroff,pendingInfo.layersize);
        destroyRsBuffer(ctx, pendingInfo.buffer);
    }
}