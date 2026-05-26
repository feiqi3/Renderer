#include "Renderer/RenderQueue.h"
#include "Renderer/MaterialInstance.h"
#include <cassert>

namespace Render {

	class IViewImpl {
	public:
		IViewImpl(const RenderQueue::PriorityMap& map)
			: mMap(map), mBeg(mMap.cbegin()), mEnd(mMap.cend()), mCur(mBeg) {
		}

		virtual const RenderCommand* next() = 0;
		virtual ~IViewImpl() = default;

	protected:
		const RenderQueue::PriorityMap& mMap; 
		RenderQueue::PriorityMap::const_iterator mBeg;
		RenderQueue::PriorityMap::const_iterator mEnd;
		RenderQueue::PriorityMap::const_iterator mCur;
	};

	class ViewImplPassNameMaskFilter : public IViewImpl {
	public:
		ViewImplPassNameMaskFilter(const RenderQueue::PriorityMap& map, Name passName, u64 mask)
			: IViewImpl(map), mName(passName), mMask(mask) {
		}

		virtual const RenderCommand* next() override {
			const RenderCommand* cmd = nullptr;
			while (mCur != mEnd) {
				cmd = &mCur->second;
				mCur++;

				if ((cmd->renderMask & mMask) != 0) {
					Pass* pass = cmd->entity->getPass(mName);
					if (pass) {
						return cmd;
					}
				}
			}
			return nullptr;
		}

	private:
		Name mName;
		u64 mMask;
	};

	class ViewImplMaskFilter : public IViewImpl {
	public:
		ViewImplMaskFilter(const RenderQueue::PriorityMap& map, u64 mask)
			: IViewImpl(map), mMaskTag(mask) {
		}

		virtual const RenderCommand* next() override {
			const RenderCommand* cmd = nullptr;
			while (mCur != mEnd) {
				cmd = &mCur->second;
				mCur++;

				if ((cmd->renderMask & mMaskTag) != 0) {
					return cmd;
				}
			}
			return nullptr;
		}
	private:
		u64 mMaskTag;
	};

	void RenderQueue::submit(RenderEntity* entity, u64 renderMask)
	{
		if (!entity || !entity->getMaterial()) return;

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
		mDp = new ViewImplPassNameMaskFilter(map, passName, tagMask);
	}

	RenderQueue::View::~View()
	{
		delete mDp;
		mDp = nullptr;
	}

	const Render::RenderCommand* RenderQueue::View::next()
	{
		assert(mDp != nullptr && "RenderQueue::View backend implementation is missing.");
		return mDp->next();
	}

	RenderQueue::View RenderQueue::getView(uint64_t tagMask) const
	{
		return View(mCommands, tagMask);
	}

	RenderQueue::View RenderQueue::getView(const Name& passName, uint64_t tagMask) const
	{
		return View(mCommands, passName, tagMask);
	}
}