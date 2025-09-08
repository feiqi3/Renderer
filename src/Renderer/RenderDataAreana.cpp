#include "Renderer/RenderDataAreana.h"
#include <cassert>
#include "vulkan_image_data.h"

namespace Render {

    FrameArenaNode::FrameArenaNode(uint32_t nodeSize)
        : mNodeSize(nodeSize)
    {
        mData = new uint8_t[nodeSize];
    }

    FrameArenaNode::~FrameArenaNode() {
        delete[] mData;
        mData = nullptr;
    }

    void FrameArenaNode::reset() {
        mHeadOffset = 0;
    }

    void FrameArenaNode::update(uint64_t frame) {
        mLastActiveFrame = frame;
    }

    // ---------------- RenderDataArena ----------------
    RenderDataArena::RenderDataArena(uint64_t perNodeSize, uint32_t frameInFlight, uint32_t maxNodeVacantFrame)
        : mPerArenaNodeSize(static_cast<uint32_t>(perNodeSize))
        , mMaxFrameInFlight(frameInFlight)
        , mMaxVacantFrame(maxNodeVacantFrame)
    {
        assert(mPerArenaNodeSize > 0);
        mInUsedArena.resize(frameInFlight);
        mFreeArena.resize(frameInFlight);
        mFrameMutex.reserve(frameInFlight);
        for (uint32_t i = 0; i < frameInFlight; ++i) {
            mFrameMutex.emplace_back(std::make_unique<std::mutex>());
        }
    }

    RenderDataArena::~RenderDataArena()
    {
        for(auto && arenaList : mInUsedArena){
            for (auto&& arena : arenaList) {
                delete arena;
            }
            arenaList.clear();
        }
        mInUsedArena.clear();
        for (auto&& arenaList : mFreeArena) {
            for (auto&& arena : arenaList) {
                delete arena;
            }
            arenaList.clear();
        }
        mFreeArena.clear();
    }

    void RenderDataArena::beginFrame(uint64_t frameIdx) {
        mCurFrame = frameIdx;
        mCurFrameInFlight = static_cast<uint32_t>(frameIdx % mMaxFrameInFlight);

        std::lock_guard<std::mutex> lock(*mFrameMutex[mCurFrameInFlight]);
        auto& usedList = mInUsedArena[mCurFrameInFlight];
        auto& freeList = mFreeArena[mCurFrameInFlight];
        cleanFrame(frameIdx);
        // 将上一帧使用过的节点移动到 free 桶
        for (auto& node : usedList) {
            node->reset();
            freeList.emplace_back(std::move(node));
        }
        usedList.clear();
    }

    AllocRange RenderDataArena::allocateFromArena(size_t reqSize) {
        assert(reqSize <= mPerArenaNodeSize && "Request size exceeds node size!");

        std::lock_guard<std::mutex> lock(*mFrameMutex[mCurFrameInFlight]);
        FrameArenaNode* node = findArenaNode(reqSize, mCurFrameInFlight);

        uint32_t offset = node->mHeadOffset;
        node->mHeadOffset += static_cast<uint32_t>(reqSize);

        AllocRange range;
        range.data = node->mData;
        range.offset = offset;
        range.size = static_cast<uint32_t>(reqSize);
        return range;
    }

    void RenderDataArena::cleanFrame(uint64_t frameIdx) {
        uint32_t frameSlot = static_cast<uint32_t>(frameIdx % mMaxFrameInFlight);

        auto& freeList = mFreeArena[frameSlot];

        for (auto it = freeList.begin(); it != freeList.end(); ) {
            if (frameIdx - (*it)->mLastActiveFrame > mMaxVacantFrame) {
                delete (*it);
                it = freeList.erase(it); // FrameArenaNode 自动析构释放 mData
            }
            else {
                ++it;
            }
        }
    }

    FrameArenaNode* RenderDataArena::findArenaNode(uint64_t reqSize, uint32_t frameIdx) {
        auto& freeList = mFreeArena[frameIdx];

        if (!freeList.empty()) {
            // 复用一个 free 节点
            FrameArenaNode* node = freeList.front();
            freeList.pop_front();
            node->reset();
            node->update(mCurFrame);
            mInUsedArena[frameIdx].push_back(node);
            return mInUsedArena[frameIdx].back();
        }

        // 没有可用节点，新建一个
        createNewNode(); // 创建后直接放入 used 桶
        return mInUsedArena[frameIdx].back();
    }

    void RenderDataArena::createNewNode() {
        FrameArenaNode* node = new FrameArenaNode(mPerArenaNodeSize);
        node->update(mCurFrame);
        mInUsedArena[mCurFrameInFlight].push_back(node);
    }

}