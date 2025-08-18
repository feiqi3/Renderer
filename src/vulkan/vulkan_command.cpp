#include"vulkan/vulkan_command.h"
#include <thread>
#include <cassert>
namespace Render::Vulkan {

    int MaxActiveFrames = 10;
	CommandBufferManager::CommandBufferManager(int maxFrame) :m_maxFrameInFlight(maxFrame)
	{
		mPools.resize(m_maxFrameInFlight);
		mThreadsCmdBuffersCache.resize(m_maxFrameInFlight);
	}

	void CommandBufferManager::beginFrame(rs_context_vk* ctx, uint64_t frame)
	{
		//clear.
		//It is promised that all operation have done when this frame begins.
		auto curFif = frame % m_maxFrameInFlight;
		{
			std::lock_guard<std::mutex> lock(mCmdBufferLock);
			ThreadVector& threadVec = mPools[curFif];

			int pos = -1;
			auto thisThreadId = std::this_thread::get_id();
			//Find current thread's pools
			for (int i = 0; i < mThreadToPoolPos.size(); ++i) {
				if (mThreadToPoolPos[i] == thisThreadId) {
					pos = i;
					break;
				}
			}

			if (pos == -1) {
				mThreadToPoolPos.push_back(thisThreadId);
				pos = mThreadToPoolPos.size() - 1;
			}
			if (threadVec.size() < pos + 1) {
				threadVec.resize(pos + 1, {});
				mThreadsCmdBuffersCache[curFif].resize(pos + 1, {});
			}
			assert(pos >= 0);

			QueueVector& queueVec = threadVec[pos];

			for (auto&& pool : queueVec) {
				vkResetCommandPool(ctx->device, (VkCommandPool)pool->native, 0);
			}
		}

		for (auto& [queueIdx,cmdBuffers] : this->mThreadsCmdBuffersCache[curFif]) {
			std::vector<VkCommandBuffer> toFrees;
			rs_commandpool_vk* pool = 0;
			auto eraseSize = std::erase_if(cmdBuffers, [&pool ,&toFrees,frame](rs_commandbuffer_vk* cmd) {
				if (frame - cmd->lastActiveFrames > MaxActiveFrames) {
					pool = cmd->pool;
					toFrees.push_back((VkCommandBuffer)cmd->native);
					delete cmd;
					return true;
				}
				return false;
			});
			if (pool && eraseSize > 0) {
				vkFreeCommandBuffers(ctx->device, (VkCommandPool)pool->native, eraseSize, toFrees.data());
			}
		}

	}

	void CommandBufferManager::endFrame(rs_context_vk* ctx, uint64_t frame)
	{
	}

	void CommandBufferManager::submitFrame(rs_context_vk* ctx, uint64_t frame)
	{
	}

	rs_commandbuffer_vk* CommandBufferManager::getCmdBufferLocalThread(rs_context_vk* ctx, uint64_t frame, QueueType queueType)
	{
		std::lock_guard<std::mutex> lock(mCmdBufferLock);
		auto curFif = frame % m_maxFrameInFlight;
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
			for (auto&& i : mThreadsCmdBuffersCache) {
				i.push_back({});
			}
			pos = mThreadToPoolPos.size() - 1;
		}

		ThreadVector& threadVec = mPools[curFif];
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
			if (ctx->computeQueue && ctx->computeQueue->queueType & queueType) {
				pool->queue = ctx->computeQueue ;
			}
			if (ctx->presentQueue && ctx->presentQueue->queueType & queueType) {
				pool->queue = ctx->presentQueue ;
			}
			if (ctx->transferQueue && ctx->transferQueue->queueType & queueType) {
				pool->queue = ctx->transferQueue ;
			}
			
			VkCommandPoolCreateInfo ci{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };

			ci.queueFamilyIndex = pool->queue->familyIndex;
			VK_CHECK(vkCreateCommandPool(ctx->device, &ci, 0, (VkCommandPool*)&pool->native), { std::abort(); });
			queueVec.push_back(pool);
		}
		auto& curThreadQueueCmdVec = mThreadsCmdBuffersCache[curFif][pos].second;
		rs_commandbuffer_vk* cmdBuffer = 0;
		for (auto&& cachedBuffer : curThreadQueueCmdVec) {
			if (cachedBuffer->lastActiveFrames != frame) {
				cmdBuffer = cachedBuffer;
				cmdBuffer->lastActiveFrames = frame;
				return cmdBuffer;
			}
		}

		//Creat buffer
		cmdBuffer = new rs_commandbuffer_vk;
		VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocInfo.           commandPool = (VkCommandPool)pool->native;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;
		vkAllocateCommandBuffers(ctx->device, &allocInfo, (VkCommandBuffer*)&cmdBuffer->native);
		cmdBuffer->isSecondary = false;
		cmdBuffer->isTransitent = true;
		cmdBuffer->pool = pool;
		cmdBuffer->queueType = pool->queue->queueType;
		mThreadsCmdBuffersCache[curFif][pos].second.push_back(cmdBuffer);
		cmdBuffer->lastActiveFrames = frame;
		return cmdBuffer;
	}

	CommandBufferManager::~CommandBufferManager()
	{
	}
}