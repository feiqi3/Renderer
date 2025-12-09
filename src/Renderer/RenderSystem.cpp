#include "Renderer/RenderSystem.h"
#include "vulkan/vulkan_render_function.h"
#include "window/render_resource_window_glfw.h"
#include "Renderer/RenderDataAreana.h"
#include "vulkan/vulkan_pipeline.h"
#include "Renderer/MaterialVarient.h"
#include "vulkan/vulkan_command.h"
#include "Renderer/RenderPassManager.h"
#include "Renderer/CameraManager.h"
#include "Renderer/Camera.h"
#include "platform/FileSystem/FileSystem.h"
#include "platform/FileSystem/WinFileSystem.h"
#include <set>
namespace Render{

	ShaderIncludeRes ShaderIncludeFindingFunction(const std::vector<std::string>& findDirectiories, const std::string& name) {
		auto FileSys = Platform::FileSystem::instance();
		Platform::FileStreamPtr fileStream = nullptr;
		std::string shaderFullDir;
		for (auto&& fileDir : findDirectiories)
		{
			shaderFullDir = fileDir + "\\" + name;
			fileStream = FileSys->openFileStream(shaderFullDir);
			if (fileStream)break;
		}
		if (!fileStream)return ShaderIncludeRes{.FindResult = false};

		auto fileSize = fileStream->getSize();
		ShaderIncludeRes res;
		res.FindResult = true;
		res.ShaderName = std::move(shaderFullDir);
		res.ShaderContent.resize(fileSize);
		fileStream->read(res.ShaderContent.data(), fileSize);
		return res;
	}

	struct EngineEvent {
		bool WindowResize = false;
		std::atomic_bool EngineIdle = false;
	};

	struct CommandPair {
		rs_commandbuffer* commandBuffer;
		std::vector<rs_semaphore*> wait;
		std::vector<rs_semaphore*> singal;
		rs_fence* fence = nullptr;
	};

	class RenderSystemPrivate {
	public:
		std::unique_ptr< RenderPassManager> mPassManager;
		std::unique_ptr<RenderDataArena> mArena;
		std::vector<CommandPair> mRenderThreadCommandBuffers;
		std::vector<CommandPair> mLogicThreadCommandBuffers;
		std::vector<rs_rendertarget*> mSwapchainRT;
		Render::Platform::Win::WinFileSystem* mWinFileSystem;
		rs_drawdata* mCurrentCameraData = nullptr;
		EngineEvent mEngineEvent;
		ShaderIncFindFunc mShaderIncludeFunction;
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
		for (int i = 0; i < sRenderSystem->getRenderContext()->maxFrameInFlight; ++i) {
			auto render_context = sRenderSystem->getRenderContext();
		}
		sRenderSystem->mDp->mPassManager = std::make_unique<RenderPassManager>();
		sRenderSystem->mDp->mArena = std::make_unique<RenderDataArena>(1024 * 64, sRenderSystem->mBackEndContext->maxSwapChainImages);
		sRenderSystem->mCurLogicFrameInFlight = 0;
		sRenderSystem->initSwapchainRT();

		new Platform::FileSystem;
		sRenderSystem->mDp->mWinFileSystem = new Render::Platform::Win::WinFileSystem();
		Platform::FileSystem::instance()->registerFileSystem(sRenderSystem->mDp->mWinFileSystem, 1);
		new CameraManager;
	}
	void RenderSystem::destroyRenderSystem()
	{
		using namespace Vulkan;
		if (!sRenderSystem)return;
		sRenderSystem->mDp->mPassManager = 0;
		delete CameraManager::instance();
		deinitVulkanBackEnd((rs_context_vk*)sRenderSystem->mBackEndContext);
		sRenderSystem->mWindow = 0;
		sRenderSystem->mBackEndContext = 0;
		Platform::FileSystem::instance()->unregisterFileSystem(sRenderSystem->mDp->mWinFileSystem);
		delete sRenderSystem->mDp->mWinFileSystem;
		delete Platform::FileSystem::instance();
		delete sRenderSystem;
		sRenderSystem = 0;
	}
	RenderSystem* RenderSystem::instance()
	{
		return sRenderSystem;
	}
	void RenderSystem::EndLogicFrame()
	{
		Vulkan::endRsFrameVk(getRenderContext());
		if (this->FenceToWaitForRender && this->currentLogicFrame > 0)
		{
			waitForFence(FenceToWaitForRender);
			resetFence(FenceToWaitForRender);
		}
		currentLogicFrame++;
		std::swap(mDp->mRenderThreadCommandBuffers, mDp->mLogicThreadCommandBuffers);
		getRenderContext()->canRenderNextFrame = true;
	}
	void RenderSystem::BeginRenderFrame()
	{
		using namespace Vulkan;
		auto ctx = getRenderContext();
		if (mUseRenderThread) {
			while (1) {
				//Wait For Logic Frame Ready
				while (!getRenderContext()->canRenderNextFrame);
				beginRsRenderFrameVk(ctx);


				auto nxtImg = waitForNextPresentImage(ctx, (Vulkan::rs_semaphore_vk*)SignalCanRenderToPresentImageSemaphore, 0);
				for (auto&& cmd : mDp->mRenderThreadCommandBuffers) {
					Vulkan::cmdSubmitCmdBuffer(getRenderContext(), (rs_commandbuffer_vk*)cmd.commandBuffer, QueueType_Graphics,  cmd.wait ,  cmd.singal , (Vulkan::rs_fence_vk*)cmd.fence );
				}
				submitToPresentImage(ctx, nxtImg, { (Vulkan::rs_semaphore_vk*)SignalCanPresentToPresentImageSemaphore });
				ctx->currentSwapchainImage = (ctx->currentSwapchainImage + 1) % ctx->maxSwapChainImages;
				if (mDp->mEngineEvent.EngineIdle) {
					Vulkan::WaitForDeviceIdel(getRenderContext());
					mDp->mEngineEvent.EngineIdle = false;
				}
				ctx->canRenderNextFrame = false;
			}
		}
		else {
			beginRsRenderFrameVk(ctx);
			auto nxtImg = waitForNextPresentImage(ctx, (Vulkan::rs_semaphore_vk*)SignalCanRenderToPresentImageSemaphore, 0);
			for (auto&& cmd : mDp->mRenderThreadCommandBuffers) {
				Vulkan::cmdSubmitCmdBuffer(getRenderContext(), (rs_commandbuffer_vk*)cmd.commandBuffer, QueueType_Graphics,  cmd.wait , cmd.singal , (Vulkan::rs_fence_vk*)cmd.fence);
			}
			submitToPresentImage(ctx, nxtImg, { (Vulkan::rs_semaphore_vk*)SignalCanPresentToPresentImageSemaphore });
			ctx->currentSwapchainImage = (ctx->currentSwapchainImage + 1) % ctx->maxSwapChainImages;
			if (mDp->mEngineEvent.EngineIdle) {
				Vulkan::WaitForDeviceIdel(getRenderContext());
				mDp->mEngineEvent.EngineIdle = false;
			}
		}


	}
	RenderPassManager* RenderSystem::getRenderPassManager() const
	{
		return mDp->mPassManager.get();
	}

	RenderPass* RenderSystem::getRenderPass(const Name& pass) 
	{
		return getRenderPassManager()->getRenderPass(pass);
	}

	rs_commandbuffer* RenderSystem::GetCommandBufferCurFrameCurThread()
	{
		auto ctx = getRenderContext();
		return ctx->cmdBufferMgr->getCmdBufferLocalThread(ctx, getNextRenderFrame(), QueueType_Graphics, false);
	}
	rs_renderpass* RenderSystem::createRenderPass(rs_rendertarget* renderTarget,const PassDesc& passDescription)
	{
		return Vulkan::createRsRenderPassVk(this->getRenderContext(), (Render::Vulkan::rs_rendertarget_vk*)renderTarget, passDescription);
	}
	void RenderSystem::destoyRenderPass(rs_renderpass* renderPass)
	{
		if (!renderPass)return;
		Vulkan::rs_renderpass_vk* rpVk = (Vulkan::rs_renderpass_vk*)renderPass;
		Vulkan::destroyRsRenderPassVk(this->getRenderContext(), rpVk);
	}
	void RenderSystem::cmdBeginRenderPass(rs_commandbuffer* cmdbuf, rs_renderpass* pass,std::vector<ClearColor>& clearColor, ClearDepthStencil& clearDs)
	{
		Vulkan::cmdBeginRenderPass((Vulkan::rs_commandbuffer_vk*)cmdbuf, (Vulkan::rs_renderpass_vk*)pass, clearColor, clearDs);
		cmdbuf->currentRenderPass = pass;
	}
	void RenderSystem::cmdEndRenderPass(rs_commandbuffer* cmdbuf)
	{
		Vulkan::cmdEndRenderPass((Vulkan::rs_commandbuffer_vk*)cmdbuf);
	}

	void RenderSystem::cmdSetScissor(rs_commandbuffer* cmdbuf, int framebufferIdx, const Rect2D& rect)
	{
		Vulkan::cmdSetScissor((Vulkan::rs_commandbuffer_vk*)cmdbuf, rect, framebufferIdx);
	}

	void RenderSystem::cmdSetViewport(rs_commandbuffer* cmdbuf, int framebufferIdx, float minDepth, float maxDepth, const Rect2D& rect)
	{
		Vulkan::cmdSetViewport((Vulkan::rs_commandbuffer_vk*)cmdbuf, rect,minDepth,maxDepth,framebufferIdx);
	}

	rs_buffer* RenderSystem::createBuffer(void* data, uint32_t size, const BufferDesc& desc)
	{
		rs_buffer* buffer = Vulkan::createRsBuffer(getRenderContext(), desc);
		if (desc.mappable) {
			Vulkan::mapRsBuffer(getRenderContext(), (Vulkan::rs_buffer_vk*)buffer);
		}
		updateBuffer(buffer, data, size, 0, true);
		return buffer;
	}


	void RenderSystem::destroyBuffer(rs_buffer* buffer)
	{
		auto rsBuffer = (Vulkan::rs_buffer_vk*)buffer;
		Vulkan::destroyRsBuffer(getRenderContext(), rsBuffer, false);
	}
	void RenderSystem::updateBuffer(rs_buffer* buffer, void* data, uint32_t size, uint32_t offset, bool imm)
	{
		if (buffer->mappedPtr) {
			memcpy(buffer->mappedPtr, data, size);
			return;
		}
		Vulkan::updateBuffer(getRenderContext(),nullptr, (Vulkan::rs_buffer_vk*)buffer, data, size, offset, imm);
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
	rs_sampler* RenderSystem::createSampler(const SamplerDesc& desc)
	{
		auto ctx = getRenderContext();
		return Vulkan::createRsSampler(ctx, desc);
	}
	void RenderSystem::destroyRsSampler(rs_sampler* sampler)
	{
		auto ctx = getRenderContext();
		auto vkSampler = (Vulkan::rs_sampler_vk*)sampler;
		Vulkan::destroyRsSampler(ctx, vkSampler);
	}
	void RenderSystem::clearDrawData(rs_drawdata* drawdata)
	{
		return Vulkan::clearDrawData(getRenderContext(),(Vulkan::rs_drawdata_vk*)drawdata);
	}
	void RenderSystem::destroyDrawData(rs_drawdata* drawdata)
	{
		return Vulkan::destroyDrawData(getRenderContext(),  (Vulkan::rs_drawdata_vk*)drawdata);
	}
	rs_drawdata* RenderSystem::createDrawData()
	{
		return Vulkan::createDrawData(getRenderContext());
	}
	rs_image* RenderSystem::createImage2D(void* data, size_t byteSize, ImageFormat format, int x, int y, int z, int layer, int mipmap)
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
			Vulkan::updateImage(getRenderContext(),nullptr, ret, data, byteSize, 0, 0, 0, x, y, z,0, 0,layer,true );
		}
		return ret;

	}

	void RenderSystem::destroyImage(rs_image* image)
	{
		auto imageVk = (Vulkan::rs_image_vk*)image;
		Vulkan::destroyRsImage(getRenderContext(), imageVk, false);
	}

	rs_image* RenderSystem::createRTTexture(ImageFormat format, int x, int y, int z, int layer, bool needSample)
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
		desc.usage = ImageUsage::ImageUsage_ColorAttachment;

		if (needSample) {
			desc.usage |= ImageUsage::ImageUsage_Sampled;
		}

		return Vulkan::createRsImage(getRenderContext(), desc);
	}
	rs_image* RenderSystem::createDepthStencilTexture(ImageFormat format, int x, int y,bool needSample)
	{
		ImageDesc desc{};
		desc.width = x;
		desc.height = y;
		desc.depth = 1;
		desc.mipLevels = 1;
		desc.arrayLayers = 1;

		ImageType type = ImageType::V2D;

		desc.type = type;
		desc.format = format;
		
		desc.usage = ImageUsage::ImageUsage_DepthStencilAttachment;
		if (needSample) {
			desc.usage |= ImageUsage::ImageUsage_Sampled;
		}
		return Vulkan::createRsImage(getRenderContext(), desc);
	}
	rs_rendertarget* RenderSystem::createRendertarget(const std::vector<rs_image*>images, rs_image* dsTex)
	{
		auto ctx = getRenderContext();
		return Vulkan::createRsRenderTarget(ctx, (Vulkan::rs_image_vk**)images.data(), images.size(), (Vulkan::rs_image_vk*)dsTex);
	}
	void RenderSystem::destroyRenderTarget(rs_rendertarget* rt)
	{
		auto ctx = getRenderContext();
		Vulkan::rs_rendertarget_vk* vkrt = (Vulkan::rs_rendertarget_vk*)rt;
		Vulkan::destroyRsRenderTarget(ctx, vkrt);
	}
	void RenderSystem::beginFrame()
	{
		if (mUseRenderThread) {
			while (mDp->mEngineEvent.EngineIdle);
		}
		//Before frame event
		if (mDp->mEngineEvent.WindowResize) {
			getRenderContext()->currentSwapchainImage = 0;
			Vulkan::createSwapchain(getRenderContext(), this->mWindow, getRenderContext()->swapchain);
			mDp->mPassManager->onSwapchainRebuild();
			mDp->mEngineEvent.WindowResize = false;
			deinitSwapchainRT();
			initSwapchainRT();
		}
		Vulkan::beginRsFrameVk(getRenderContext());
		mDp->mLogicThreadCommandBuffers.clear();
		mCurLogicFrameInFlight = currentLogicFrame % getRenderContext()->maxFrameInFlight;
		mDp->cleanUpFramesPendingData(currentLogicFrame % mBackEndContext->maxFrameInFlight, currentLogicFrame);
		mDp->mCurrentCameraData = nullptr;
		getRenderPassManager()->onFrameBegin();
	}

	rs_binding_pos RenderSystem::getBindingPos(const std::string& bindingName, Material* material)
	{
		auto data = material->getBindingInfoByName(bindingName);
		if (data.has_value()) {
			return (*data).bindingPos;
		}
		return INVALID_BINDING_POS;
	}

	void RenderSystem::updateUniformBufferData(rs_binding_pos binding, void* data, uint32_t size, Pass* pass)
	{	
		auto ctx = getRenderContext();
		auto pipeline = (Vulkan::rs_pipeline_vk*)pass->mMaterial->getRsPipeline();
		auto drawData = (Vulkan::rs_drawdata_vk*)pass->mDrawData;
		Vulkan::updateDrawData(ctx, ctx->nextRenderFrame,pipeline,drawData, binding, data, size);
	}
	void RenderSystem::updateUniform(rs_binding_pos binding, rs_buffer* buffer, Pass* pass)
	{
		auto ctx = getRenderContext();
		auto pipeline = (Vulkan::rs_pipeline_vk*)pass->mMaterial->getRsPipeline();
		auto drawData = (Vulkan::rs_drawdata_vk*)pass->mDrawData;
		Vulkan::updateDrawData(ctx, ctx->nextRenderFrame, pipeline, drawData, binding, (Vulkan::rs_buffer_vk*)buffer);
	}
	void RenderSystem::updateUniform(rs_binding_pos binding, rs_image* image, Pass* pass)
	{
		auto ctx = getRenderContext();
		auto pipeline = (Vulkan::rs_pipeline_vk*)pass->mMaterial->getRsPipeline();
		auto drawData = (Vulkan::rs_drawdata_vk*)pass->mDrawData;
		Vulkan::updateDrawData(ctx, ctx->nextRenderFrame,pipeline,drawData, binding, (Vulkan::rs_image_vk*)image);

	}
	void RenderSystem::updateUniform(rs_binding_pos binding, rs_sampler* sampler, Pass* pass)
	{
		auto ctx = getRenderContext();
		auto pipeline = (Vulkan::rs_pipeline_vk*)pass->mMaterial->getRsPipeline();
		auto drawData = (Vulkan::rs_drawdata_vk*)pass->mDrawData;
		Vulkan::updateDrawData(ctx, ctx->nextRenderFrame, pipeline, drawData, binding, (Vulkan::rs_sampler_vk*)sampler);
	}
	void RenderSystem::updateImageData(rs_image* image, void* data,size_t byteSize, int x, int y, int z, int width, int height, int depth, int layerOffset, int layerSize, int mip)
	{
		auto ctx = getRenderContext();
		Vulkan::updateImage(ctx,nullptr, (Vulkan::rs_image_vk*)image, data, byteSize, x, y, z, width, height, depth, mip, layerOffset, layerSize, false);
	}
	void RenderSystem::updateBufferData(rs_buffer* buffer, void* data, size_t byteSize, size_t dstOffset)
	{
		auto ctx = getRenderContext();
		Vulkan::updateBuffer(getRenderContext(),nullptr, (Vulkan::rs_buffer_vk*)buffer, data, byteSize, dstOffset, false);
	}
	void* RenderSystem::placeFramePendingData(void* data, uint32_t size)
	{
		auto fif = currentLogicFrame % mBackEndContext->maxFrameInFlight;
		return mDp->updateFramePendingData(fif, currentLogicFrame, data, size);
	}
	void RenderSystem::submitCmdBuffer(rs_commandbuffer* cmdBuffer, const std::vector<rs_semaphore*>& wait, const std::vector<rs_semaphore*>& singal, rs_fence* fence)
	{
		if (cmdBuffer->hasCommands == false) {
			//return;
		}
		cmdBuffer->hasCommands = false;
		
		CommandPair pair{
			.commandBuffer = cmdBuffer,
			.wait = wait,
			.singal = singal,
			.fence = fence
		};
		mDp->mLogicThreadCommandBuffers.push_back(pair);
	}
	void RenderSystem::drawIndexed(rs_commandbuffer* cmdBuffer, RenderEntity* entity,const Name& passName)
	{
		auto pass = entity->getPass(passName);
		if (!pass)return;

		drawIndexed(cmdBuffer, entity, pass);
	}
	void RenderSystem::drawIndexed(rs_commandbuffer* cmdBuffer, RenderEntity* entity, Pass* pass)
	{
		cmdBuffer->hasCommands = true;
		auto pipeline = (Vulkan::rs_pipeline_vk*)pass->mMaterial->getRsPipeline();
		auto entityDrawData = (Vulkan::rs_drawdata_vk*)pass->mDrawData;
		std::array<Vulkan::rs_drawdata_vk*,3> drawDataArr{};
		drawDataArr[0] = (Vulkan::rs_drawdata_vk*)entityDrawData;
		drawDataArr[2] = (Vulkan::rs_drawdata_vk*)mDp->mCurrentCameraData;
		Vulkan::cmdDrawIndexed((Vulkan::rs_commandbuffer_vk*)cmdBuffer, pipeline, entity->getRenderInfo(), drawDataArr, getCurFif());
	}
	void RenderSystem::waitForFence(rs_fence* fence)
	{
		Vulkan::waitForRsFence(getRenderContext(), (Vulkan::rs_fence_vk*)fence,-1,getRenderContext()->curRenderFrame % getRenderContext()->maxFrameInFlight);
	}
	void RenderSystem::clearRenderEntity(RenderEntity* entity)
	{
		for (auto& [passName, pass] : entity->mPasses) {
			auto drawDataVk = (Vulkan::rs_drawdata_vk*)pass->mDrawData;
			Vulkan::clearDrawData(getRenderContext(),drawDataVk);
		}
	}
	ShaderIncFindFunc RenderSystem::getShaderIncludeSearchFunc() const
	{
		return &ShaderIncludeFindingFunction;
	}
	
	const std::vector<std::string>& RenderSystem::getShaderIncludeSearchDir() const
	{
		static const std::vector<std::string> ret{ "../shader","shader" };
		return ret;
	}

	uint64_t RenderSystem::getNextRenderFrame() const
	{
		return currentLogicFrame;
	}
	uint32_t RenderSystem::getCurFif() const
	{
		return mCurLogicFrameInFlight;
	}
	rs_image* RenderSystem::getSwapchainImage(uint32_t idx)
	{
		return getRenderContext()->swapchain->swapchainImgs[idx];
	}
	rs_rendertarget* RenderSystem::getNextSwapchainRendertarget()
	{
		auto ctx = getRenderContext();
		uint32_t NxtImageIdx = ctx->currentSwapchainImage;
		return mDp->mSwapchainRT[NxtImageIdx];
	}

	void RenderSystem::initSwapchainRT()
	{
		auto ctx = getRenderContext();

		for (int i = 0; i < ctx->maxSwapChainImages; ++i) {
			mDp->mSwapchainRT.push_back(createRendertarget({ getSwapchainImage(i) }, 0));
		}
	}

	void RenderSystem::deinitSwapchainRT()
	{
		for (auto&& rt : mDp->mSwapchainRT) {
			destroyRenderTarget(rt);
		}
		mDp->mSwapchainRT.clear();
	}

	void RenderSystem::cmdBegin(rs_commandbuffer* cmdBuffer)
	{
		Vulkan::cmdBeginRecord((Vulkan::rs_commandbuffer_vk*)cmdBuffer);
	}

	void RenderSystem::cmdEnd(rs_commandbuffer* cmdBuffer)
	{
		Vulkan::cmdEndRecord((Vulkan::rs_commandbuffer_vk*)cmdBuffer);
	}

	rs_semaphore* RenderSystem::createSemphore()
	{
		return Vulkan::createRsSemaphore(getRenderContext());
	}

	void RenderSystem::destroySemphore(rs_semaphore* semphore)
	{
		auto vkSem = (Vulkan::rs_semaphore_vk*)semphore;
		Vulkan::destroyRsSemaphore(getRenderContext(), vkSem);
	}

	rs_fence* RenderSystem::createFence()
	{
		auto ret = Vulkan::createRsFence(this->getRenderContext());
		return ret;
	}

	void RenderSystem::destroyFence(rs_fence* fence)
	{
		auto fenceVk = (Vulkan::rs_fence_vk*) fence;
		Vulkan::destroyRsFence(getRenderContext(), fenceVk);
	}

	void RenderSystem::resetFence(rs_fence* fence)
	{
		Vulkan::resetRsFence(getRenderContext(), (Vulkan::rs_fence_vk*)fence, getRenderContext()->curRenderFrame % getRenderContext()->maxFrameInFlight);
	}

	void RenderSystem::setCurrentCamera(Camera* camera)
	{
		if (camera->getCameraActive() == false) {
			assert(0 && "Camera not active");

		}
		mDp->mCurrentCameraData = CameraManager::instance()->updateCameraDrawData(camera);
	}

	void RenderSystem::setEngineIdle()
	{
		mDp->mEngineEvent.EngineIdle = true;
	}

	void RenderSystem::waitForEngineIdle()
	{
		Vulkan::WaitForDeviceIdel(getRenderContext());
	}

	void RenderSystem::onWindowResize()
	{
		setEngineIdle();
		mDp->mEngineEvent.WindowResize = true;
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