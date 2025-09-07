#ifndef RENDER_DATA_AREANA_H_
#define RENDER_DATA_AREANA_H_
#include "common/RingBuffer.h"
#include "common/NoCopyable.h"
#include <vector>
#include <list>
namespace Render {
	struct FrameArenaNode{
		FrameArenaNode(uint32_t nodeSize);
		~FrameArenaNode();
		void reset();
		void update(uint64_t frame);

		uint64_t mLastActiveFrame = 0;
		uint8_t* mData = 0;
		uint32_t mHeadOffset = 0;
		uint32_t mNodeSize = 1024 * 8; //default 8KB
	};


	struct AllocRange {
		uint8_t* data;
		uint32_t offset;
		uint32_t size;
	};

	class RenderDataArena : public Common::NonCopyable {
	public:
		RenderDataArena(uint64_t perNodeSize,uint32_t frameInFlight,uint32_t maxNodeVacantFrame = 10);
		~RenderDataArena();
		void cleanFrame(uint64_t frameIdx);
		void beginFrame(uint64_t frameIdx);
		AllocRange allocateFromArena(size_t reqSize);
	private:
		void createNewNode();

		uint32_t mPerArenaNodeSize = 0;
		FrameArenaNode* findArenaNode(uint64_t reqSize,uint32_t frameIdx);
		using AreaNodeList = std::list<FrameArenaNode*>;
		std::vector<AreaNodeList> mInUsedArena;
		std::vector<AreaNodeList> mFreeArena;
		std::vector<std::unique_ptr<std::mutex>> mFrameMutex;
		uint64_t mCurFrame = 0;
		uint32_t mMaxFrameInFlight = 2;
		uint32_t mCurFrameInFlight = 0;
		uint32_t mMaxVacantFrame = 10;
	};
};

#endif