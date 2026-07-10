#ifndef VULKAN_COMMAND_H
#define VULKAN_COMMAND_H

#include "vulkan_render_resource.h"
#include <thread>
#include <mutex>
#include <vector>
#include <list>

namespace Render::Vulkan {

	class CommandBufferManager {
	public:
		//Per Queue
		struct CommandPoolBlock {
			QueueType queueType;
			rs_commandpool_vk* pool = nullptr;
			std::list<rs_commandbuffer_vk*> freeList;
			std::list<rs_commandbuffer_vk*> usedList;
		};

		//Per Thread
		struct ThreadData {
			std::thread::id threadId;
			std::vector<CommandPoolBlock> poolBlocks;
		};

		//Per Frame
		struct FrameData {
			std::list<ThreadData> threads;
		};

	public:
		CommandBufferManager(int maxFrame);
		~CommandBufferManager();

		void beginFrame(rs_context_vk* ctx, uint64_t frame);
		rs_commandbuffer_vk* getCmdBufferLocalThread(rs_context_vk* ctx, uint64_t frame, QueueType queueType, bool singleTime);
		rs_commandbuffer_vk* getCmdBufferCurRenderThread(rs_context_vk* ctx, QueueType queueType, bool singleTime);
		void clearAll(rs_context_vk* ctx);

	private:
		int m_maxFrameInFlight;
		std::mutex mCmdBufferLock;
		std::vector<FrameData> m_frames;
	};
}
#endif