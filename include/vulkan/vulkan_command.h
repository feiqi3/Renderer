#ifndef VULKAN_COMMAND_H
#define VULKAN_COMMAND_H
#include "vulkan_render_resource.h"
#include <thread>
#include <mutex>
namespace Render::Vulkan {

	class CommandBufferManager {
	public:
	
		using QueueVector = std::vector<rs_commandpool_vk*>;
		using ThreadVector = std::vector<QueueVector>;
		using FrameVector = std::vector<ThreadVector>;

		using QueueCmdVector = std::pair<uint8_t,std::vector< rs_commandbuffer_vk*>>;
		CommandBufferManager(int maxFrame);
		void beginFrame(rs_context_vk* ctx, uint64_t frame);
		void endFrame(rs_context_vk* ctx, uint64_t frame);
		void submitFrame(rs_context_vk* ctx, uint64_t frame);
		rs_commandbuffer_vk* getCmdBufferLocalThread(rs_context_vk* ctx, uint64_t frame, QueueType queueType);
		
		~CommandBufferManager();
	private:
		std::vector<std::thread::id> mThreadToPoolPos;
		FrameVector mPools;
		std::vector<std::vector<rs_commandbuffer_vk*>> mToDestroyCmdBuffers;
		std::vector<QueueCmdVector> mCmdsToSubmit;
		int m_maxFrameInFlight;
		std::mutex mCmdBufferLock;
	};

}

#endif