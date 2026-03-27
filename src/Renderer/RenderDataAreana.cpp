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
    }

    void RenderDataArena::shutdown() {
        for (auto& frame : m_frames) {
            for (auto* block : frame.blocks) {
                destroyBlock(block);
            }
            frame.blocks.clear();
            frame.pendingCopies.clear();
        }
        m_frames.clear();
    }

    void RenderDataArena::beginFrame(uint64_t frameIndex) {
        m_currentFrameIndex = frameIndex % m_maxFramesInFlight;
        FrameData& frame = m_frames[m_currentFrameIndex];


        std::vector<StagingBlock*> blocksToErase;
        
        std::erase_if(frame.blocks, [&](StagingBlock* block) {
            if (frameIndex - block->lastUsedFrame > m_maxVacantFrames) {
                destroyBlock(block);
                return true;
            }
            return false;
		});

        for (auto* block : frame.blocks) {
            if(frame.pendingCopies.size() == 0){
                block->currentOffset = 0;
            }
        }
        frame.activeBlockIndex = 0;
    }

    void RenderDataArena::allocateStagingMemory(uint32_t size, uint32_t frameIndex, rs_buffer** outBuffer, uint32_t* outOffset, void** outMappedPtr) {
        FrameData& frame = m_frames[m_currentFrameIndex];

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
        *outBuffer = targetBlock->buffer;
        *outOffset = targetBlock->currentOffset;
        *outMappedPtr = (uint8_t*)targetBlock->mappedPtr + targetBlock->currentOffset;

        targetBlock->currentOffset += size;
    }

    void RenderDataArena::stageBufferUpdate(rs_buffer* dstBuffer, uint32_t dstOffset, const void* data, uint32_t size) {
        if (size == 0) return;

        rs_buffer* srcBuf = nullptr;
        uint32_t srcOffset = 0;
        void* mapPtr = nullptr;

        allocateStagingMemory(size, RenderSystem::instance()->getNextRenderFrame(), &srcBuf, &srcOffset, &mapPtr);

        memcpy(mapPtr, data, size);

        FrameData& frame = m_frames[m_currentFrameIndex];
        CopyCommand cmd;
        cmd.srcBuffer = srcBuf;
        cmd.dstBuffer = dstBuffer;
        cmd.srcOffset = srcOffset;
        cmd.dstOffset = dstOffset;
        cmd.size = size;

        frame.pendingCopies.push_back(cmd);
    }

    void RenderDataArena::executePendingCopies(rs_commandbuffer* cmd) {
        FrameData& frame = m_frames[m_currentFrameIndex];
        for (auto block : frame.blocks) {
            bool needFlushThis = block->lastUsedFrame == RenderSystem::instance()->getNextRenderFrame();
            if (needFlushThis) {
                RenderSystem::instance()->flushBuffer(block->buffer, block->buffer->byteSize);
            }
        }
        if (frame.pendingCopies.empty()) return;
        //Excute copies
        std::atomic_thread_fence(std::memory_order_seq_cst);
        for (const auto& copy : m_frames[m_currentFrameIndex].pendingCopies){
			RenderSystem::instance()->cmdCopyBufferToBuffer(cmd, copy.srcBuffer, copy.dstBuffer, copy.srcOffset, copy.dstOffset, copy.size);
        }
        m_frames[m_currentFrameIndex].pendingCopies.clear();
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