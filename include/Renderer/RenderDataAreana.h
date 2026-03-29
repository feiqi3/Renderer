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
        void stageBufferUpdate(rs_image* dstImage, const void* data, uint32_t size);
        void stageBufferUpdate(rs_image* dstImage, const void* data, uint32_t size,
            int x, int y, int z, int width, int height, int depth,
            uint32_t mipOffset,uint32_t mipCount, int layeroff, int layerSize);
        void stageBufferUpdate(rs_buffer* dstBuffer, uint32_t dstOffset, const void* data, uint32_t size);
    private:
        void copyDataToStageBuffer(const void* data, uint32_t size, rs_buffer** usedStageBuffer, uint32_t* outSrcOffset);
    private:

        struct StagingBlock {
            rs_buffer*  buffer = nullptr; 
            void*       mappedPtr = nullptr;   
            uint32_t    capacity = 0;
            uint32_t    currentOffset = 0;
            uint64_t    lastUsedFrame = 0;
        };

        struct CopyBufferToBufferCommand {
            rs_buffer*  srcBuffer;
            rs_buffer*  dstBuffer;
            uint32_t    srcOffset;
            uint32_t    dstOffset;
            uint32_t    size;
        };

        struct CopyBufferToImageCommand {
            rs_buffer*  srcBuffer;
            rs_image*   dstImage;
            uint32_t    srcOffset;
            int         x, y, z;
            int         width, height, depth;
            uint32_t    mipOffset;
            uint32_t    mipCount;
            int         layeroff;
            int         layerSize;
        };
        
        struct FrameData {
            std::vector<StagingBlock*> blocks;

            std::vector<CopyBufferToBufferCommand> pendingBufferCopies;
            std::vector<CopyBufferToImageCommand> pendingImageCopies;

            uint32_t activeBlockIndex = 0;
        };

        void allocateStagingMemory(uint32_t size, uint32_t frameIndex, rs_buffer** outBuffer, uint32_t* outOffset, void** outMappedPtr);
        void allocateDedicateMemory(uint32_t size, uint32_t frameIndex, rs_buffer** outBuffer, uint32_t* outOffset, void** outMappedPtr);
        StagingBlock* createBlock(uint32_t size);
        void destroyBlock(StagingBlock* block);

    private:
        uint32_t m_maxFramesInFlight = 0;
        uint32_t m_currentFiF = 0;
		uint32_t m_stageBlockSize    = STAGING_BLOCK_SIZE;
        uint32_t m_alignSize         = 256;
        //A buffer can exist for m_maxVacantFrames after last used
        uint32_t m_maxVacantFrames = 5;
        uint64_t m_currentFrame = 0;
        std::vector<FrameData> m_frames;
        std::vector<FrameData> m_dedicateBufferFrames;
    };
}

#endif