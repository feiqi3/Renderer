#ifndef RENDER_DATA_ARENA_H
#define RENDER_DATA_ARENA_H

#include "RenderSystem.h"
#include <vector>
#include <list>

namespace Render {

    constexpr uint32_t STAGING_BLOCK_SIZE = 1024 * 1024 * 16;

    class RenderDataArena {
    public:
        RenderDataArena();
        ~RenderDataArena();

        // 初始化
        void init(uint32_t maxFramesInFlight,uint32_t stageBlockSize);
        void shutdown();

        void beginFrame(uint64_t frameIndex);

        void executePendingCopies(rs_commandbuffer* cmd);

        void stageBufferUpdate(rs_buffer* dstBuffer, uint32_t dstOffset, const void* data, uint32_t size);

    private:
        struct StagingBlock {
            rs_buffer* buffer = nullptr; 
            void* mappedPtr = nullptr;   
            uint32_t capacity = 0;
            uint32_t currentOffset = 0;
            uint64_t lastUsedFrame = 0;
        };

        struct CopyCommand {
            rs_buffer* srcBuffer;
            rs_buffer* dstBuffer;
            uint32_t srcOffset;
            uint32_t dstOffset;
            uint32_t size;
        };

        struct FrameData {
            std::vector<StagingBlock*> blocks;

            std::vector<CopyCommand> pendingCopies;

            uint32_t activeBlockIndex = 0;
        };

        void allocateStagingMemory(uint32_t size, uint32_t frameIndex, rs_buffer** outBuffer, uint32_t* outOffset, void** outMappedPtr);

        StagingBlock* createBlock(uint32_t size);
        void destroyBlock(StagingBlock* block);

    private:
        uint32_t m_maxFramesInFlight = 0;
        uint32_t m_currentFrameIndex = 0;
		uint32_t m_stageBlockSize = STAGING_BLOCK_SIZE;
        //A buffer can exist for m_maxVacantFrames after last used
        uint32_t m_maxVacantFrames = 5;
        std::vector<FrameData> m_frames;
    };
}

#endif