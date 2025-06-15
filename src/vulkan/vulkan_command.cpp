#include"vulkan/vulkan_command.h"
#include <thread>
namespace Render::Vulkan {
	CommandBufferManager::CommandBufferManager(int maxFrame) :m_maxFrameInFlight(maxFrame)
	{
		mPools.resize(m_maxFrameInFlight);
	}

	void CommandBufferManager::beginFrame(rs_context_vk* ctx, uint64_t frame)
	{
		//clear.
		//It is promised that all operation have done when this frame begins.
		{
			std::lock_guard<std::mutex> lock(mCmdBufferLock);

			int pos = -1;
			auto thisThreadId = std::this_thread::get_id();
			//Find current thread's pools
			for (int i = 0; i < mThreadToPoolPos.size(); ++i) {
				if (mThreadToPoolPos[i] == thisThreadId) {
					pos = i;
					break;
				}
			}

			assert(pos >= 0);

			ThreadVector& threadVec = mPools[frame % m_maxFrameInFlight];
			QueueVector& queueVec = threadVec[pos];

			for (auto&& pool : queueVec) {
				vkResetCommandPool(ctx->device, (VkCommandPool)pool->native, 0);
			}
		}
		for (auto&& [queueType, cmds] : mCmdsToSubmit) {
			for (auto cb : cmds) {
				delete cb;
				cb = 0;
			}
		}
	}

	void CommandBufferManager::endFrame(rs_context_vk* ctx, uint64_t frame)
	{
	}

	void CommandBufferManager::submitFrame(rs_context_vk* ctx, uint64_t frame)
	{
		//TODO
		std::lock_guard<std::mutex> lock(mCmdBufferLock);
		for (auto&& [queue, cmds] : mCmdsToSubmit) {
			VkSubmitInfo info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
			uint32_t                       waitSemaphoreCount;
			const VkSemaphore* pWaitSemaphores;
			const VkPipelineStageFlags* pWaitDstStageMask;
			uint32_t                       commandBufferCount;
			const VkCommandBuffer* pCommandBuffers;
			uint32_t                       signalSemaphoreCount;
			const VkSemaphore* pSignalSemaphores;
		}
	}

	rs_commandbuffer_vk* CommandBufferManager::getCmdBufferLocalThread(rs_context_vk* ctx, uint64_t frame, QueueType queueType)
	{
		std::lock_guard<std::mutex> lock(mCmdBufferLock);
		int pos = -1;
		auto thisThreadId = std::this_thread::get_id();
		//Find current thread's pools
		for (int i = 0; i < mThreadToPoolPos.size();++i) {
			if (mThreadToPoolPos[i] == thisThreadId) {
				pos = i;
				break;
			}
		}
		if (pos == -1) {
			mThreadToPoolPos.push_back(thisThreadId);
			for (auto&& i : mPools) {
				i.push_back({});
			}
			pos = mThreadToPoolPos.size() - 1;
		}

		ThreadVector& threadVec = mPools[frame % m_maxFrameInFlight];
		QueueVector& queueVec = threadVec[pos];

		//Find target pool with identical queueType
		rs_commandpool_vk* pool = 0;
		for (auto&& i : queueVec) {
			if (i && i->queue->queueType & queueType) {
				pool = i;
				break;
			}
		}
		if (!pool) {
			pool = new rs_commandpool_vk;
			if (ctx->graphicQueue->queueType & queueType) {
				pool->queue = ctx->graphicQueue ;
			}
			if (ctx->computeQueue->queueType & queueType) {
				pool->queue = ctx->computeQueue ;
			}
			if (ctx->presentQueue->queueType & queueType) {
				pool->queue = ctx->presentQueue ;
			}
			if (ctx->transferQueue->queueType & queueType) {
				pool->queue = ctx->transferQueue ;
			}
			
			VkCommandPoolCreateInfo ci{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };

			ci.queueFamilyIndex = pool->queue->familyIndex;
			VK_CHECK(vkCreateCommandPool(ctx->device, &ci, 0, (VkCommandPool*)&pool->native), { std::abort(); });
			queueVec.push_back(pool);
		}

		//Creat buffer
		rs_commandbuffer_vk* cmdBuffer = new rs_commandbuffer_vk;
		VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocInfo.           commandPool = (VkCommandPool)pool->native;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;
		vkAllocateCommandBuffers(ctx->device, &allocInfo, (VkCommandBuffer*)&cmdBuffer->native);
		cmdBuffer->isSecondary = false;
		cmdBuffer->isTransitent = true;
		cmdBuffer->pool = pool;
		cmdBuffer->queueType = pool->queue->queueType;
		
		int queueCmdsPos = -1;
		for (int i = 0; i < mCmdsToSubmit.size(); ++i) {
			if (mCmdsToSubmit[i].first == pool->queue->queueType) {
				queueCmdsPos = i;
				break;
			}
		}

		if (queueCmdsPos == -1) {
			mCmdsToSubmit.push_back({ pool->queue->queueType,{} });
			queueCmdsPos = mCmdsToSubmit.size() - 1;
		}

		mCmdsToSubmit[queueCmdsPos].second.push_back(cmdBuffer);
		return cmdBuffer;
	}

	CommandBufferManager::~CommandBufferManager()
	{
	}
}