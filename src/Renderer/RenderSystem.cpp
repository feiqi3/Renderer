#include "Renderer/RenderSystem.h"
#include "vulkan/vulkan_render_function.h"
#include "window/render_resource_window_glfw.h"
#include "common/RingBuffer.h"
namespace Render{

	struct PendingBuffer {
		uint8_t* pendingDataPtr = 0;
		uint32_t size = 0;
		uint32_t pos = 0;
		uint64_t lastActiveFrame = 0;
	};



	class RenderSystemPrivate {
	public:
		std::vector<
			std::list< PendingBuffer>
		> mFramesPendingData;
		std::mutex mFrameDataMutex;

	public:
		void* updateFramePendingData(uint32_t fif, uint64_t frame, void* data, uint32_t size);
		void cleanUpFramesPendingData(uint32_t fif, uint64_t frame);
	};

	RenderSystem* RenderSystem::sRenderSystem = nullptr;
	void RenderSystem::createRenderSystem(const BackEndInitDesc& backEndDesc, Window::rs_window* window)
	{
		using namespace Window;
		using namespace Vulkan;
		if (sRenderSystem) {
			return;
		}
		sRenderSystem = new RenderSystem;
		sRenderSystem->mWindow = window;
		sRenderSystem->mBackEndContext = initVulkanBackEnd(backEndDesc, window);
	}
	void RenderSystem::destroyRenderSystem()
	{
		using namespace Vulkan;
		if (!sRenderSystem)return;
		deinitVulkanBackEnd((rs_context_vk*)sRenderSystem->mBackEndContext);
		sRenderSystem->mWindow = 0;
		sRenderSystem->mBackEndContext = 0;
		delete sRenderSystem;
		sRenderSystem = 0;
	}
	RenderSystem* RenderSystem::instance()
	{
		return sRenderSystem;
	}
	void RenderSystem::beginFrame()
	{
		currentLogicFrame++;
		mDp->cleanUpFramesPendingData(mBackEndContext->maxFrameInFlight, currentLogicFrame);
	}
	void* RenderSystem::placeFramePendingData(void* data, uint32_t size)
	{
		auto fif = currentLogicFrame % mBackEndContext->maxFrameInFlight;
		return mDp->updateFramePendingData(fif, currentLogicFrame, data, size);
	}
	RenderSystem::RenderSystem()
	{
		mDp = std::make_unique< RenderSystemPrivate>();
		mDp->mFramesPendingData.resize(mBackEndContext->maxFrameInFlight);
	}
	RenderSystem::~RenderSystem()
	{

	}
	void* RenderSystemPrivate::updateFramePendingData(uint32_t fif, uint64_t frame, void* data, uint32_t size)
	{
		const uint32_t BufferSize = 1024 * 64; //64k
		//1. find first avail buffer

		PendingBuffer* pendingBuffer = 0;

		std::lock_guard<std::mutex> lock(mFrameDataMutex);

		auto& framePendingDatas = mFramesPendingData[fif];
		for (auto&& data : framePendingDatas) {
			if (data.size < size) {
				throw std::runtime_error("Large Data Error!");
				return nullptr;
			}
			if (data.size - data.pos < size) {
				continue;
			}
			else {
				pendingBuffer = &data;
				pendingBuffer->lastActiveFrame = frame;
			}
		}

		if (!pendingBuffer) {
			PendingBuffer buffer{};
			buffer.pendingDataPtr = new uint8_t[BufferSize];
			buffer.pos = 0;
			buffer.size = BufferSize;
			framePendingDatas.push_front(buffer);
			pendingBuffer = &framePendingDatas.front();
		}
		pendingBuffer->lastActiveFrame = frame;
		pendingBuffer->pos += size;
		memcpy(pendingBuffer->pendingDataPtr, data, size);
		return pendingBuffer->pendingDataPtr;

	}
	void RenderSystemPrivate::cleanUpFramesPendingData(uint32_t fif,uint64_t frame)
	{
		auto& framePendingBuffers = mFramesPendingData[fif];
		const int FRAME_VACANT_MAX = 10;
		for (auto itor = framePendingBuffers.begin(); itor != framePendingBuffers.end();) {
			if (frame - itor->lastActiveFrame > FRAME_VACANT_MAX) {
				itor = framePendingBuffers.erase(itor);
			}
			else {
				itor->pos = 0; //Reset.
				itor++;
			}
		}
	}
}