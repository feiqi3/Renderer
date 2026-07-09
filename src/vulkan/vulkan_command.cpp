#include "vulkan/vulkan_command.h"
#include "render_log.h"
#include "vulkan/vulkan_render_function.h" 
#include <thread>
#include <cassert>

namespace Render::Vulkan {

	static const int MaxActiveFrames = 10;

	CommandBufferManager::CommandBufferManager(int maxFrame) : m_maxFrameInFlight(maxFrame)
	{
		m_frames.resize(m_maxFrameInFlight);
	}

	CommandBufferManager::~CommandBufferManager()
	{
	}

	void CommandBufferManager::beginFrame(rs_context_vk* ctx, uint64_t frame)
	{
		auto curFif = frame % m_maxFrameInFlight;

		std::lock_guard<std::mutex> lock(mCmdBufferLock);
		FrameData& frameData = m_frames[curFif];

		for (auto& threadData : frameData.threads) {
			for (auto& block : threadData.poolBlocks) {
				if (!block.pool) continue;

				vkResetCommandPool(ctx->device, (VkCommandPool)block.pool->native, 0);

				if (!block.usedList.empty()) {
					block.freeList.splice(block.freeList.end(), block.usedList);
				}

				while (!block.freeList.empty()) {
					rs_commandbuffer_vk* coldCmd = block.freeList.front();

					//The old one is still active, skip checking the young
					if (frame - coldCmd->lastActiveFrames <= MaxActiveFrames) {
						break;
					}

					vkFreeCommandBuffers(ctx->device,
						(VkCommandPool)block.pool->native,
						1,
						(VkCommandBuffer*)&coldCmd->native);
					delete coldCmd;

					block.freeList.pop_front();
				}
			}
		}
	}


	Render::Vulkan::rs_commandbuffer_vk* CommandBufferManager::getCmdBufferCurRenderThread(rs_context_vk* ctx, QueueType queueType, bool singleTime)
	{
		auto frameIdx = ctx->curRenderFrame;
		auto curFif = frameIdx % m_maxFrameInFlight;
		ThreadData* currentThreadData = nullptr;
		{
			//Find target queue lists in current thread
			std::lock_guard<std::mutex> lock(mCmdBufferLock);
			FrameData& frameData = m_frames[curFif];
			auto thisThreadId = std::this_thread::get_id();

			for (auto& tData : frameData.threads) {
				if (tData.threadId == thisThreadId) {
					currentThreadData = &tData;
					break;
				}
			}
			if (!currentThreadData) {
				frameData.threads.emplace_back();
				currentThreadData = &frameData.threads.back();
				currentThreadData->threadId = thisThreadId;
			}
		}

		CommandPoolBlock* targetBlock = nullptr;
		//find target command list in target queue
		for (auto& block : currentThreadData->poolBlocks) {
			if (block.queueType & queueType) {
				targetBlock = &block;
				break;
			}
		}

		if (!targetBlock) {
			currentThreadData->poolBlocks.emplace_back();
			targetBlock = &currentThreadData->poolBlocks.back();
			targetBlock->queueType = queueType;

			rs_commandpool_vk* pool = new rs_commandpool_vk;
			rs_queue_vk* targetQueue = ctx->graphicQueue;
			if (ctx->computeQueue && (ctx->computeQueue->queueType & queueType)) targetQueue = ctx->computeQueue;
			if (ctx->transferQueue && (ctx->transferQueue->queueType & queueType)) targetQueue = ctx->transferQueue;

			pool->queue = targetQueue;
			VkCommandPoolCreateInfo ci{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
			ci.queueFamilyIndex = pool->queue->familyIndex;
			ci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

			VK_CHECK(vkCreateCommandPool(ctx->device, &ci, 0, (VkCommandPool*)&pool->native), { std::abort(); });
			targetBlock->pool = pool;
		}

		rs_commandbuffer_vk* cmdBuffer = nullptr;

		//Find a command buffer in free list
		if (!targetBlock->freeList.empty()) {
			auto it = std::prev(targetBlock->freeList.end());
			cmdBuffer = *it;

			targetBlock->usedList.splice(targetBlock->usedList.end(),
				targetBlock->freeList,
				it);
		}
		else {
			//Create one if there are no vacant one
			cmdBuffer = new rs_commandbuffer_vk;
			VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
			allocInfo.commandPool = (VkCommandPool)targetBlock->pool->native;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = 1;

			vkAllocateCommandBuffers(ctx->device, &allocInfo, (VkCommandBuffer*)&cmdBuffer->native);

			cmdBuffer->pool = targetBlock->pool;
			cmdBuffer->queueType = targetBlock->pool->queue->queueType;

			targetBlock->usedList.push_back(cmdBuffer);
		}

		cmdBuffer->isSecondary = false;
		cmdBuffer->isTransitent = singleTime;
		cmdBuffer->lastActiveFrames = frameIdx;

		return cmdBuffer;
	}



	rs_commandbuffer_vk* CommandBufferManager::getCmdBufferLocalThread(rs_context_vk* ctx, uint64_t frame, QueueType queueType, bool singleTime)
	{
		ThreadData* currentThreadData = nullptr;
		{
			//Find target queue lists in current thread
			std::lock_guard<std::mutex> lock(mCmdBufferLock);

			auto curFif = frame % m_maxFrameInFlight;
			FrameData& frameData = m_frames[curFif];
			auto thisThreadId = std::this_thread::get_id();

			for (auto& tData : frameData.threads) {
				if (tData.threadId == thisThreadId) {
					currentThreadData = &tData;
					break;
				}
			}
			if (!currentThreadData) {
				frameData.threads.emplace_back();
				currentThreadData = &frameData.threads.back();
				currentThreadData->threadId = thisThreadId;
			}
		}

		CommandPoolBlock* targetBlock = nullptr;
		//find target command list in target queue
		for (auto& block : currentThreadData->poolBlocks) {
			if (block.queueType & queueType) {
				targetBlock = &block;
				break;
			}
		}

		if (!targetBlock) {
			currentThreadData->poolBlocks.emplace_back();
			targetBlock = &currentThreadData->poolBlocks.back();
			targetBlock->queueType = queueType;

			rs_commandpool_vk* pool = new rs_commandpool_vk;
			rs_queue_vk* targetQueue = ctx->graphicQueue;
			if (ctx->computeQueue && (ctx->computeQueue->queueType & queueType)) targetQueue = ctx->computeQueue;
			if (ctx->transferQueue && (ctx->transferQueue->queueType & queueType)) targetQueue = ctx->transferQueue;

			pool->queue = targetQueue;
			VkCommandPoolCreateInfo ci{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
			ci.queueFamilyIndex = pool->queue->familyIndex;
			ci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

			VK_CHECK(vkCreateCommandPool(ctx->device, &ci, 0, (VkCommandPool*)&pool->native), { std::abort(); });
			targetBlock->pool = pool;
		}

		rs_commandbuffer_vk* cmdBuffer = nullptr;

		//Find a command buffer in free list
		if (!targetBlock->freeList.empty()) {
			auto it = std::prev(targetBlock->freeList.end());
			cmdBuffer = *it;

			targetBlock->usedList.splice(targetBlock->usedList.end(),
				targetBlock->freeList,
				it);
		}
		else {
			//Create one if there are no vacant one
			cmdBuffer = new rs_commandbuffer_vk;
			VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
			allocInfo.commandPool = (VkCommandPool)targetBlock->pool->native;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = 1;

			vkAllocateCommandBuffers(ctx->device, &allocInfo, (VkCommandBuffer*)&cmdBuffer->native);

			cmdBuffer->pool = targetBlock->pool;
			cmdBuffer->queueType = targetBlock->pool->queue->queueType;

			targetBlock->usedList.push_back(cmdBuffer);
		}

		cmdBuffer->isSecondary = false;
		cmdBuffer->isTransitent = singleTime;
		cmdBuffer->lastActiveFrames = frame; 

		return cmdBuffer;
	}

	void CommandBufferManager::clearAll(rs_context_vk* ctx) {
		std::lock_guard<std::mutex> lock(mCmdBufferLock);

		for (auto& frameData : m_frames) {
			for (auto& threadData : frameData.threads) {
				for (auto& block : threadData.poolBlocks) {
					if (block.pool) {
						for (auto cmd : block.freeList) delete cmd;
						block.freeList.clear();

						for (auto cmd : block.usedList) delete cmd;
						block.usedList.clear();

						vkDestroyCommandPool(ctx->device, (VkCommandPool)block.pool->native, 0);
						delete block.pool;
					}
				}
			}
			frameData.threads.clear();
		}
	}
}