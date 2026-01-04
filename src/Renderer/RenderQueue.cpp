#include "Renderer/RenderQueue.h"

namespace Render {
    void RenderQueue::submit(RenderEntity* entity, u32 renderOrder, u64 renderMask)
    {
        RenderCommand cmd{};
        cmd.entity = entity;
        cmd.renderMask = renderMask;
        cmd.renderOrder = renderOrder;
        mCommands.emplace(cmd.renderOrder, cmd);
    }

    void RenderQueue::clear()
    {
        mCommands.clear();
    }

    size_t RenderQueue::size() const
    {
        return mCommands.size();
    }

    RenderQueue::View::View(const PriorityMap& map, uint64_t tagMask)
        : mMap(map), mIt(map.begin()), mEnd(map.end()), mTagMask(tagMask)
    {
    }


    const RenderCommand* RenderQueue::View::next()
    {
        while (mIt != mEnd)
        {
            const RenderCommand* candidate = &mIt->second;
            ++mIt; 
            if ((candidate->renderMask & mTagMask) != 0)
            {
                return candidate;
            }
        }
        return nullptr;
    }

    RenderQueue::View RenderQueue::getView(uint64_t tagMask) const
    {
        return View(mCommands, tagMask);
    }

    RenderQueue& RenderGroup::getQueue(const Name& passName)
    {
        return mPasses[passName];
    }

    const RenderQueue* RenderGroup::getQueue(const Name& passName) const
    {
        auto it = mPasses.find(passName);
        if (it == mPasses.end()) return nullptr;
        return &(it->second);
    }

    void RenderGroup::clear()
    {
        for (auto& [name, queue] : mPasses) {
            queue.clear();
        }
    }
}