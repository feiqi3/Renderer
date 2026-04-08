#ifndef RENDER_QUEUE_H_
#define RENDER_QUEUE_H_
#include "RenderEntity.h"
#include "common/CoreDefs.h"
#include "common/Name.h"
#include <map>
#include "Renderer/RenderCommand.h"
#include <unordered_map>
namespace Render {

    namespace RenderOrder {
        u32 Opaque = 1000;
        u32 SkyBox = 9999;
        u32 Transparent = 10000;
    };

    namespace RenderMask {
        u64 Normal = 1ull << 0;
		u64 SkyBox = 1ull << 32;
    };

    class Pass;
    class RenderQueue
    {
    public:
        using PriorityMap = std::multimap<uint32_t, RenderCommand>; // key = renderOrder

        RenderQueue() = default;
        ~RenderQueue() = default;

        RenderQueue(const RenderQueue&) = delete;
        RenderQueue& operator=(const RenderQueue&) = delete;

        void submit(RenderEntity* entity,u64 renderMask = UINT64_MAX);

        void clear();

        size_t size() const;

        class View
        {
        public:
            View(const PriorityMap& map, uint64_t tagMask = UINT64_MAX);
            View(const PriorityMap& map,const Name& passName, uint64_t tagMask = UINT64_MAX);
            ~View();
            const RenderCommand* next();

        private:
            class IViewImpl* mDp = 0;
        };

        View getView(uint64_t tagMask = UINT64_MAX) const;
        View getView(const Name& name, uint64_t tagMask = UINT64_MAX)const;
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