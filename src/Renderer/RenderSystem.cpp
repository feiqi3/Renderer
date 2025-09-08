#include "Renderer/RenderSystem.h"
#include "vulkan/vulkan_render_function.h"
#include "window/render_resource_window_glfw.h"
#include "Renderer/RenderDataAreana.h"
namespace Render{



	class RenderSystemPrivate {
	public:

		std::unique_ptr<RenderDataArena> mArena;

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

		sRenderSystem->mDp->mArena = std::make_unique<RenderDataArena>(1024 * 64, sRenderSystem->mBackEndContext->maxSwapChainImages);
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
		
	}
	RenderSystem::~RenderSystem()
	{

	}

	void* RenderSystemPrivate::updateFramePendingData(uint32_t fif, uint64_t frame, void* data, uint32_t size)
	{
		const uint32_t BufferSize = 1024 * 64; //64k

		auto allocInfo = mArena->allocateFromArena(size);
		if (allocInfo.data) {
			memcpy(allocInfo.data + allocInfo.offset, data, size);
		}
		else {
			throw std::runtime_error("No Valid Space for Render Data");
		}
		return allocInfo.data + allocInfo.offset;
	}
	void RenderSystemPrivate::cleanUpFramesPendingData(uint32_t fif,uint64_t frame)
	{
		mArena->beginFrame(frame);
	}
}