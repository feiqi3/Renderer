#include "Renderer/RenderSystem.h"
#include "vulkan/vulkan_render_function.h"
#include "window/render_resource_window_glfw.h"
#include "Renderer/RenderDataAreana.h"
#include "vulkan/vulkan_pipeline.h"
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
	rs_renderpass* RenderSystem::createRenderPass(rs_rendertarget* renderTarget, PassDesc& passDescription)
	{
		return Vulkan::createRsRenderPassVk(this->getRenderContext(), (Render::Vulkan::rs_rendertarget_vk*)renderTarget, passDescription);
	}
	void RenderSystem::destoyRenderPass(rs_renderpass* renderPass)
	{
		Vulkan::rs_renderpass_vk* rpVk = (Vulkan::rs_renderpass_vk*)renderPass;
		Vulkan::destroyRsRenderPassVk(this->getRenderContext(), rpVk);
	}
	rs_pipeline* RenderSystem::createRenderPipeline(rs_renderpass* renderpass, PipelineDesc& pipelineDescription)
	{
		return Vulkan::createRsPipeline(getRenderContext(), (Vulkan::rs_renderpass_vk*)renderpass, pipelineDescription);
	}
	void RenderSystem::destroyRenderPipeline(rs_pipeline* pipeline)
	{
		auto plVk = (Vulkan::rs_pipeline_vk*)pipeline;
		Vulkan::destroyRsPipeline(getRenderContext(), plVk);
	}
	rs_image* RenderSystem::createImage2D(void* data, size_t byteSize, ImageFormat format, int x, int y, int z, int layer, int layersize, int mipmap)
	{
		ImageDesc desc{};
		desc.width = x;
		desc.height = y;
		desc.depth = z;
		desc.mipLevels = 1;
		desc.arrayLayers = layer;

		ImageType type = ImageType::V2D;
		if (z > 1) {
			type = ImageType::V3D;
		}
		else if (layer > 1) {
			type = ImageType::V2D_Array;
		}

		desc.type = type;
		desc.format = format;
		desc.usage = ImageUsage::ImageUsage_Sampled;

		auto ret = Vulkan::createRsImage(getRenderContext(), desc);
		if (data) {
			Vulkan::updateImage(getRenderContext(), ret, data, byteSize, 0, 0, 0, x, y, z, 0,layer,layersize,true );
		}
		return ret;

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