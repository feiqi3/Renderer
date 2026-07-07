#include "Renderer/RenderQueue.h"
#include "Renderer/MaterialInstance.h"
#include <cassert>

namespace Render {

	class IViewImpl {
	public:
		virtual const RenderCommand* next() = 0;
		virtual ~IViewImpl() = default;
	};

	class BucketViewImpl : public IViewImpl {
	public:
		BucketViewImpl(std::vector<const std::vector<RenderCommand>*>&& buckets)
			: mBuckets(std::move(buckets)), mBucketIdx(0), mCmdIdx(0), mHasPassFilter(false), mPassName("") {
		}

		BucketViewImpl(std::vector<const std::vector<RenderCommand>*>&& buckets, const Name& passName)
			: mBuckets(std::move(buckets)), mBucketIdx(0), mCmdIdx(0), mHasPassFilter(true), mPassName(passName) {
		}

		virtual const RenderCommand* next() override {
			while (mBucketIdx < mBuckets.size()) {
				const auto& bucket = *mBuckets[mBucketIdx];

				if (mCmdIdx < bucket.size()) {
					const auto& cmd = bucket[mCmdIdx++];

					if (mHasPassFilter) {
						if (cmd.entity->getPass(mPassName) != nullptr) {
							return &cmd;
						}
					}
					else {
						return &cmd;
					}
				}
				else {
					mBucketIdx++;
					mCmdIdx = 0;
				}
			}
			return nullptr;
		}

	private:
		std::vector<const std::vector<RenderCommand>*> mBuckets;
		size_t mBucketIdx;
		size_t mCmdIdx;
		bool mHasPassFilter;
		Name mPassName;
	};


	void RenderQueue::submit(RenderEntity* entity, u64 renderMask)
	{
		if (!entity || !entity->getMaterial()) return;

		RenderCommand cmd{};
		cmd.entity = entity;
		cmd.worldPos = entity->getWorldPos();

		u64 mask = renderMask;
		uint32_t bitIdx = 0;
		while (mask > 0 && bitIdx < BUCKET_COUNT) {
			if (mask & 1ull) {
				mBuckets[bitIdx].push_back(cmd);
				mTotalSize++;
			}
			mask >>= 1;
			bitIdx++;
		}
	}

	void RenderQueue::clear()
	{
		for (size_t i = 0; i < BUCKET_COUNT; ++i) {
			mBuckets[i].clear();
		}
		mTotalSize = 0;
	}

	size_t RenderQueue::size(RenderMaskID maskID) const
	{
		size_t idx = static_cast<size_t>(maskID);
		if (idx < BUCKET_COUNT) {
			return mBuckets[idx].size();
		}
		return 0;
	}

	RenderQueue::View::View(std::vector<const std::vector<RenderCommand>*>&& buckets)
	{
		mDp = new BucketViewImpl(std::move(buckets));
	}

	RenderQueue::View::View(std::vector<const std::vector<RenderCommand>*>&& buckets, const Name& passName)
	{
		mDp = new BucketViewImpl(std::move(buckets), passName);
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
		std::vector<const std::vector<RenderCommand>*> activeBuckets;
		u64 mask = tagMask;
		uint32_t bitIdx = 0;
		while (mask > 0 && bitIdx < BUCKET_COUNT) {
			if (mask & 1ull) {
				if (!mBuckets[bitIdx].empty()) {
					activeBuckets.push_back(&mBuckets[bitIdx]);
				}
			}
			mask >>= 1;
			bitIdx++;
		}
		return View(std::move(activeBuckets));
	}

	RenderQueue::View RenderQueue::getView(const Name& passName, uint64_t tagMask) const
	{
		std::vector<const std::vector<RenderCommand>*> activeBuckets;
		u64 mask = tagMask;
		uint32_t bitIdx = 0;
		while (mask > 0 && bitIdx < BUCKET_COUNT) {
			if (mask & 1ull) {
				if (!mBuckets[bitIdx].empty()) {
					activeBuckets.push_back(&mBuckets[bitIdx]);
				}
			}
			mask >>= 1;
			bitIdx++;
		}
		return View(std::move(activeBuckets), passName);
	}
}