#ifndef RENDER_QUEUE_H_
#define RENDER_QUEUE_H_

#include "RenderEntity.h"
#include "common/CoreDefs.h"
#include "common/Name.h"
#include "Renderer/RenderCommand.h"
#include <vector>

namespace Render {

	namespace RenderOrder {
		inline u32 Opaque = 1000;
		inline u32 SkyBox = 9999;
		inline u32 Transparent = 10000;
	};

	enum class RenderMaskID {
		Normal = 0,
		ShadowCaster = 1,
		Transparent = 16,
		Skybox = 32,
		DebugDraw = 50,
		GUI2D = 51
	};

	namespace RenderMask {
		inline u64 Normal = 1ull << (int)RenderMaskID::Normal;
		inline u64 ShadowCaster = 1ull << (int)RenderMaskID::ShadowCaster;
		inline u64 Transparent = 1ull << (int)RenderMaskID::Transparent;
		inline u64 SkyBox = 1ull << (int)RenderMaskID::Skybox;
		inline u64 DebugDraw = 1ull << (int)RenderMaskID::DebugDraw;
		inline u64 Gui2D = 1ull << (int)RenderMaskID::GUI2D;
	};

	class Pass;

	class RenderQueue
	{
	public:
		RenderQueue() = default;
		~RenderQueue() = default;

		RenderQueue(const RenderQueue&) = delete;
		RenderQueue& operator=(const RenderQueue&) = delete;

		void submit(RenderEntity* entity, u64 renderMask = 0);

		void clear();
		size_t size(RenderMaskID maskID) const;

		class View
		{
		public:
			View(std::vector<const std::vector<RenderCommand>*>&& buckets);
			View(std::vector<const std::vector<RenderCommand>*>&& buckets, const Name& passName);
			~View();
			const RenderCommand* next();

		private:
			class IViewImpl* mDp = nullptr;
		};

		View getView(uint64_t tagMask = UINT64_MAX) const;
		View getView(const Name& name, uint64_t tagMask = UINT64_MAX) const;

	private:
		static constexpr size_t BUCKET_COUNT = 64;

		mutable std::vector<RenderCommand> mBuckets[BUCKET_COUNT];
		mutable bool mBucketSorted[BUCKET_COUNT] = { false };

		size_t mTotalSize = 0;
	};
}

#endif // RENDER_QUEUE_H_