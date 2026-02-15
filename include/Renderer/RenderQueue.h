#ifndef RENDER_QUEUE_H_
#define RENDER_QUEUE_H_
#include "RenderEntity.h"
#include "common/CoreDefs.h"
#include "common/Name.h"
#include <map>
#include "Renderer/RenderCommand.h"
#include <unordered_map>
namespace Render {
    class Pass;
    class RenderQueue
    {
    public:
        using PriorityMap = std::multimap<uint32_t, RenderCommand>; // key = renderOrder

        RenderQueue() = default;
        ~RenderQueue() = default;

        RenderQueue(const RenderQueue&) = delete;
        RenderQueue& operator=(const RenderQueue&) = delete;

        void submit(RenderEntity* entity,Pass* pass,u64 renderMask = UINT64_MAX);

        void clear();

        size_t size() const;

        class View
        {
        public:
            View(const PriorityMap& map, uint64_t tagMask = UINT64_MAX);

            const RenderCommand* next();

        private:
            const PriorityMap& mMap;
            PriorityMap::const_iterator mIt;
            PriorityMap::const_iterator mEnd;
            uint64_t mTagMask;
            bool mHasPrepared = false;     
            bool mHasCachedNext = false; 
        };

        View getView(uint64_t tagMask = UINT64_MAX) const;

    private:
        PriorityMap mCommands;
    };


    class RenderGroup
    {
    public:
        RenderGroup() = default;
        ~RenderGroup() = default;

        RenderGroup(const RenderGroup&) = delete;
        RenderGroup& operator=(const RenderGroup&) = delete;
        RenderQueue& getQueue(const Name& queueName);

        const RenderQueue* getQueue(const Name& queueName) const;

        void clear();


    private:
        std::unordered_map<Name, RenderQueue> mPasses;
    };
}

#endif