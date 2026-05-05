#include "Renderer/RenderDataAreana.h"

namespace Render {

    RenderDataArena::RenderDataArena() {}

    RenderDataArena::~RenderDataArena() {
        shutdown();
    }

    void RenderDataArena::init(uint32_t maxFramesInFlight, uint32_t stageBlockSize) {
        m_maxFramesInFlight = maxFramesInFlight;
        m_stageBlockSize = stageBlockSize;
        m_frames.resize(m_maxFramesInFlight);
        m_dedicateBufferFrames.resize(m_maxFramesInFlight);
    }

    void RenderDataArena::shutdown() {
        for (auto& frame : m_frames) {
            for (auto* block : frame.blocks) {
                destroyBlock(block);
            }
            frame.blocks.clear();
            frame.pendingBufferCopies.clear();
            frame.pendingImageCopies.clear();
        }
        for (auto& frame : m_dedicateBufferFrames) {
            for (auto* block : frame.blocks) {
                destroyBlock(block);
            }
            frame.blocks.clear();
        }
        m_dedicateBufferFrames.clear();

        m_frames.clear();
    }

    void RenderDataArena::beginFrame(uint64_t frameIndex) {
        m_currentFrame = frameIndex;
        m_currentFiF = frameIndex % m_maxFramesInFlight;
        FrameData& frame = m_frames[m_currentFiF];


        std::vector<StagingBlock*> blocksToErase;
        
        std::erase_if(frame.blocks, [&](StagingBlock* block) {
            if (frameIndex - block->lastUsedFrame > m_maxVacantFrames) {
                destroyBlock(block);
                return true;
            }
            return false;
		});

        for (auto* block : frame.blocks) {
            if(frame.pendingBufferCopies.size() == 0){
                block->currentOffset = 0;
            }
        }
        frame.activeBlockIndex = 0;

        FrameData& dedFrame = m_dedicateBufferFrames[m_currentFiF];
        std::erase_if(dedFrame.blocks, [&](StagingBlock* block) {
            if (frameIndex - block->lastUsedFrame > m_maxVacantFrames) {
                destroyBlock(block);
                return true;
            }
            return false;
            });

        for (auto* block : dedFrame.blocks) {
            if (dedFrame.pendingBufferCopies.empty() && dedFrame.pendingImageCopies.empty()) {
                block->currentOffset = 0;
            }
        }
        dedFrame.activeBlockIndex = 0;

    }

    void RenderDataArena::allocateStagingMemory(uint32_t size, uint32_t frameIndex, rs_buffer** outBuffer, uint32_t* outOffset, void** outMappedPtr) {
        FrameData& frame = m_frames[m_currentFiF];

        StagingBlock* targetBlock = nullptr;

        if (frame.activeBlockIndex < frame.blocks.size()) {
            StagingBlock* cur = frame.blocks[frame.activeBlockIndex];
            uint32_t alignedOffset = (cur->currentOffset + 255) & ~255;

            if (alignedOffset + size <= cur->capacity) {
                targetBlock = cur;
                targetBlock->currentOffset = alignedOffset;
            }
            else {
                frame.activeBlockIndex++;
            }
        }

        if (!targetBlock && frame.activeBlockIndex >= frame.blocks.size()) {
            uint32_t allocSize = std::max(size, STAGING_BLOCK_SIZE);
            targetBlock = createBlock(allocSize);
            frame.blocks.push_back(targetBlock);
        }
        else if (!targetBlock) {
            targetBlock = frame.blocks[frame.activeBlockIndex];
            targetBlock->currentOffset = 0;
        }
        targetBlock->lastUsedFrame = frameIndex;
		targetBlock->generationMark++;

        assert(targetBlock->capacity - targetBlock->currentOffset >= size);

        //NOTICE: ITS OK HERE, SINCE WE CAN ASSURE THIS BUFFER IS NOT TOUCHED BY GPU NOW
        targetBlock->buffer->state = ResourceState::HostWrite;
        *outBuffer = targetBlock->buffer;
        *outOffset = targetBlock->currentOffset;
        *outMappedPtr = (uint8_t*)targetBlock->mappedPtr + targetBlock->currentOffset;
        targetBlock->currentOffset += size;
    }

    void RenderDataArena::allocateDedicateMemory(uint32_t size, uint32_t frameIndex, rs_buffer** outBuffer, uint32_t* outOffset, void** outMappedPtr)
    {
        FrameData& frame = m_dedicateBufferFrames[m_currentFiF];

        StagingBlock* targetBlock = nullptr;

        if (frame.activeBlockIndex < frame.blocks.size()) {
            StagingBlock* cur = frame.blocks[frame.activeBlockIndex];
            uint32_t alignedOffset = (cur->currentOffset + 255) & ~255;

            if (alignedOffset + size <= cur->capacity) {
                targetBlock = cur;
                targetBlock->currentOffset = alignedOffset;
            }
            else {
                frame.activeBlockIndex++;
            }
        }

        if (!targetBlock && frame.activeBlockIndex >= frame.blocks.size()) {
            uint32_t allocSize = std::max(size, STAGING_BLOCK_SIZE);
            targetBlock = createBlock(allocSize);
            frame.blocks.push_back(targetBlock);
        }
        else if (!targetBlock) {
            targetBlock = frame.blocks[frame.activeBlockIndex];
            targetBlock->currentOffset = 0;
        }
        targetBlock->lastUsedFrame = frameIndex;

        //NOTICE: ITS OK HERE, SINCE WE CAN ASSURE THIS BUFFER IS NOT TOUCHED BY GPU NOW
        targetBlock->buffer->state = ResourceState::HostWrite;
        *outBuffer = targetBlock->buffer;
        *outOffset = targetBlock->currentOffset;
        *outMappedPtr = (uint8_t*)targetBlock->mappedPtr + targetBlock->currentOffset;

        targetBlock->currentOffset += size;
    }

    void RenderDataArena::stageBufferUpdate(rs_buffer* dstBuffer, uint32_t dstOffset, const void* data, uint32_t size) {
        if (size == 0) return;

        rs_buffer* srcBuf = nullptr;
        uint32_t srcOffset = 0;
        copyDataToStageBuffer(data, size, &srcBuf, &srcOffset);
        FrameData& frame = m_frames[m_currentFiF];
        CopyBufferToBufferCommand cmd;
        cmd.srcBuffer = srcBuf;
        cmd.dstBuffer = dstBuffer;
        cmd.srcOffset = srcOffset;
        cmd.dstOffset = dstOffset;
        cmd.size = size;

        frame.pendingBufferCopies.push_back(cmd);
    }

    void RenderDataArena::copyDataToStageBuffer(const void* data, uint32_t size, rs_buffer** usedStageBuffer, uint32_t* outSrcOffset)
    {
        void* mapPtr = nullptr;
        if (size > m_stageBlockSize + m_alignSize) {
            allocateDedicateMemory(size, m_currentFrame, usedStageBuffer, outSrcOffset, &mapPtr);
        }
        else
        {
            allocateStagingMemory(size, m_currentFrame, usedStageBuffer, outSrcOffset, &mapPtr);
        }
        memcpy(mapPtr, data, size);

    }

    void RenderDataArena::executePendingCopies(rs_commandbuffer* cmd) {
        FrameData& frame = m_frames[m_currentFiF];
        for (auto block : frame.blocks) {
            bool needFlushThis = block->generationMark == block->lastUpdateGenerationMark;
            if (needFlushThis) {
				block->lastUpdateGenerationMark = block->generationMark;
                RenderSystem::instance()->flushBuffer(block->buffer, block->buffer->byteSize);
            }
        }
        if (frame.pendingBufferCopies.empty()) return;
        //Excute copies
        std::atomic_thread_fence(std::memory_order_seq_cst);
        for (const auto& copy : m_frames[m_currentFiF].pendingBufferCopies){
			RenderSystem::instance()->cmdCopyBufferToBuffer(cmd, copy.srcBuffer, copy.dstBuffer, copy.srcOffset, copy.dstOffset, copy.size);
        }
        m_frames[m_currentFiF].pendingBufferCopies.clear();


        for (const auto& copy : frame.pendingImageCopies) {
            RenderSystem::instance()->cmdCopyBufferToImage(
                cmd,
                copy.srcBuffer, copy.dstImage, copy.srcOffset,
                copy.x, copy.y, copy.z,
                copy.width, copy.height, copy.depth,copy.mipOffset,
                copy.mipCount, copy.layeroff, copy.layerSize
            );
        }
        frame.pendingImageCopies.clear();

    }

    void RenderDataArena::stageBufferUpdate(rs_image* dstImage, const void* data, uint32_t size)
    {

        if (!dstImage) return;
        stageBufferUpdate(dstImage, data, size,
            0, 0, 0,
            dstImage->width, dstImage->height, dstImage->depth,
            0,dstImage->mipLevels,
            0, dstImage->arrayLayers);
    }

    void RenderDataArena::stageBufferUpdate(rs_image* dstImage, const void* data, uint32_t size,
        int x, int y, int z, int width, int height, int depth,
        uint32_t mipOffset, uint32_t mipCount, int layeroff, int layerSize)
    {

        if (size == 0 || !data) return;

        rs_buffer* srcBuf = nullptr;
        uint32_t srcOffset = 0;
        copyDataToStageBuffer(data, size, &srcBuf, &srcOffset);

        FrameData& frame = m_frames[m_currentFiF];
        CopyBufferToImageCommand cmd;
        cmd.srcBuffer = srcBuf;
        cmd.dstImage = dstImage;
        cmd.srcOffset = srcOffset;

        cmd.x = x; cmd.y = y; cmd.z = z;
        cmd.width = width; cmd.height = height; cmd.depth = depth;
        cmd.mipOffset = mipOffset;
        cmd.mipCount = mipCount;
        cmd.layeroff = layeroff; cmd.layerSize = layerSize;

        frame.pendingImageCopies.push_back(cmd);

    }


    RenderDataArena::StagingBlock* RenderDataArena::createBlock(uint32_t size) {
        StagingBlock* block = new StagingBlock();
        block->capacity = size;
        block->currentOffset = 0;
        auto ctx = RenderSystem::instance()->getRenderContext();
        BufferDesc desc{};
        desc.bufUsage = BufferType::BufferType_TransferSrc;
        desc.byteSize = size;
        desc.mappable = true;
        //FIX ME: ignore queue type now.....   
        block->buffer = RenderSystem::instance()->createBuffer(nullptr, size, desc);
        block->mappedPtr = block->buffer->mappedPtr;
        return block;
    }

    void RenderDataArena::destroyBlock(StagingBlock* block) {
        if (block) {
            RenderSystem::instance()->destroyBuffer(block->buffer);
            delete block;
        }
    }
}