#include "Renderer/RenderQueue.h"
#include "Renderer/MaterialInstance.h"

namespace Render {

    class IViewImpl {
    public:
        IViewImpl(const RenderQueue::PriorityMap& map):mMap(map),mBeg(mMap.begin()),mEnd(mMap.end()),mCur(mBeg) {}
        virtual const RenderCommand* next() = 0;
        ~IViewImpl() = default;
    protected:
        RenderQueue::PriorityMap mMap;
        RenderQueue::PriorityMap::iterator mBeg;
        RenderQueue::PriorityMap::iterator mEnd;
        RenderQueue::PriorityMap::iterator mCur;
    };
    class ViewImplPassNameMaskFilter : public IViewImpl {
    public:
        ViewImplPassNameMaskFilter(const RenderQueue::PriorityMap& map,Name passName, u64 mask):IViewImpl(map),mName(passName),mMask(mask) {
        }

        virtual const RenderCommand* next() {
            RenderCommand* cmd = 0;
            while (mCur != mEnd) {
                cmd = &mCur->second;
                mCur++;
                if (cmd->renderMask & mMask) {
                    Pass* pass = cmd->entity->getPass(mName);
                    if (pass) {
                        return cmd;
                    }
                }
            }
            return nullptr;
        }

        Name mName;
        u64 mMask;
    };
    class ViewImplMaskFilter : public IViewImpl {
        public:
            ViewImplMaskFilter(const RenderQueue::PriorityMap& map, u64 mask) :IViewImpl(map), mMaskTag(mask) {
            }
            virtual const RenderCommand* next() {
                RenderCommand* cmd = nullptr;
                while (mCur != mEnd) {
                    cmd = &mCur->second;
                    mCur++;
                    if (cmd->renderMask & mMaskTag) {
                        return cmd;
                    }
                }
                return nullptr;
            }
    private:
        RenderQueue::PriorityMap::iterator saveItor;
        u64 mMaskTag;
    };
    void RenderQueue::submit(RenderEntity* entity, u64 renderMask = UINT64_MAX)
    {
        RenderCommand cmd{};
        cmd.entity = entity;
        cmd.renderMask = renderMask;
		cmd.worldPos = entity->getWorldPos();
        mCommands.emplace(entity->getMaterial()->getRenderOrder(), cmd);
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
    {
        mDp = new ViewImplMaskFilter(map, tagMask);
    }

    RenderQueue::View::View(const PriorityMap& map, const Name& passName, uint64_t tagMask)
    {
        mDp = new ViewImplPassNameMaskFilter(map,passName, tagMask);
    }

    RenderQueue::View::~View()
    {
        delete mDp;
        mDp = 0;
    }

    RenderQueue::View RenderQueue::getView(uint64_t tagMask) const
    {
        return View(mCommands, tagMask);
    }

    RenderQueue::View RenderQueue::getView(const Name& name, uint64_t tagMask) const
    {
        return View(mCommands,name, tagMask);
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