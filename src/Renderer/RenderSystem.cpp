#include "Renderer/RenderSystem.h"
#include "render_function.h"
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
#include "Renderer/RenderQueue.h"
#include "Renderer/ConstShaderDataManager.h"
#include "Renderer/TextureResourceMgr.h"
#include "Renderer/Blit.h"
#include "common/ResourceSystem.h"
#include "Renderer/MaterialInstance.h"
#include "Renderer/MaterialTemplateManager.h"
#include "vulkan/vulkan_resource_state.h"
#include "function/EngineResourceManager.h"
#include "function/ComponentSystem.h"
#include "render_resource_global.h"
#include "Renderer/ComputeKernel.h"
#include <memory>
#include <set>
namespace Render{
	TexturePtr geterrorTexture(class RenderSystemPrivate* dp);

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
		bool IsEngineInBackEnd = false;
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
		rs_commandbuffer* mSubmitToPresentCmdBuffer = nullptr;
		std::vector<rs_rendertarget*> mSwapchainRT;
		Blitor* mBlitor = nullptr;
		TexturePtr mErrorRGBTexture = nullptr;

		Render::Platform::Win::WinFileSystem* mWinFileSystem;
		rs_drawdata* mCurrentCameraData = nullptr;
		EngineEvent mEngineEvent;
		ShaderIncFindFunc mShaderIncludeFunction;
		Camera* mCurrentCamera = nullptr;
		rs_bindless_data* globalBindlessData = nullptr;
		float globalViewPortZNear = 0.;
		float globalViewPortZFar =  1.;
		std::unique_ptr< ComputeKernel> clearRTCompute;
		rs_rendertarget* mPresentRT = nullptr;
		rs_image* mPresentImage = nullptr;
		rs_image* mRenderThreadPresentImage = nullptr;
		rs_commandbuffer* mRenderThreadCmdBuffer = nullptr;
		rs_semaphore* mRenderFinishSemaphore = nullptr;
		rs_fence*	  mRenderEndFence = nullptr;
		std::vector<std::pair<rs_image*, rs_commandbuffer*>> mFiFToBlitToSwapchain;
		int mRenderResolutionX = 1000;
		int mRenderResolutionY = 600;
	public:
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
		auto render_context = sRenderSystem->getRenderContext();
		sRenderSystem->mDp->mPassManager = std::make_unique<RenderPassManager>();
		sRenderSystem->mDp->mArena = std::make_unique<RenderDataArena>();
		sRenderSystem->mDp->mArena->init(sRenderSystem->getRenderContext()->maxFrameInFlight, Render::STAGING_BLOCK_SIZE);
		//In stage: cause we use blit function to fill the swapchain image
		//So wait in stage Resource Transfer stage
		sRenderSystem->mDp->mRenderFinishSemaphore = sRenderSystem->createSemaphore(SemaphoreWait::CurRenderFrame,ResourceState::TransferDst);
		sRenderSystem->SemaphorePresentImageReady = sRenderSystem->createSemaphore(SemaphoreWait::CurRenderFrame, ResourceState::TransferDst);
		sRenderSystem->mDp->mRenderEndFence		   = sRenderSystem->createFence();
		sRenderSystem->SemaphoreBlitToPresentImageReady = sRenderSystem->createSemaphore(SemaphoreWait::CurRenderFrame, ResourceState::Common);
		sRenderSystem->mCurLogicFrameInFlight = 0;
		sRenderSystem->initSwapchainRT();
		sRenderSystem->createPresentRT();
		new Platform::FileSystem;
		sRenderSystem->mDp->mWinFileSystem = new Render::Platform::Win::WinFileSystem();
		Platform::FileSystem::instance()->registerFileSystem(sRenderSystem->mDp->mWinFileSystem, 1);
		RegisterAllEngineResourceManager();
		new ConstShaderDataManager;
		new CameraManager;
		//TODO move it out;
		new ComponentSystem;
		sRenderSystem->mDp->mBlitor = new Blitor();
	}
	void RenderSystem::destroyRenderSystem()
	{
		using namespace Vulkan;
		if (!sRenderSystem)return;
		delete sRenderSystem->mDp->mBlitor;
		sRenderSystem->mDp->clearRTCompute = nullptr;
		delete ComponentSystem::instance();
		delete CameraManager::instance();
		delete ConstShaderDataManager::instance();
		UnRegisterAllEngineResourceManager();
		sRenderSystem->mDp->mPassManager = 0;
		sRenderSystem->mDp->mArena->shutdown();
		sRenderSystem->mDp->mArena = 0;
		sRenderSystem->destroyFence(sRenderSystem->mDp->mRenderEndFence);
		sRenderSystem->destroySemaphore(sRenderSystem->mDp->mRenderFinishSemaphore);
		sRenderSystem->destroySemaphore(sRenderSystem->SemaphorePresentImageReady);
		sRenderSystem->destroySemaphore(sRenderSystem->SemaphoreBlitToPresentImageReady);

		sRenderSystem->destroyPresentRT();
		sRenderSystem->deinitSwapchainRT();

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
		ComponentSystem::instance()->doDestroyComponents();
		mDp->mSubmitToPresentCmdBuffer = this->GetCommandBufferCurFrameCurThread();
		Vulkan::endRsFrameVk(getRenderContext());

		currentLogicFrame++;
		std::swap(mDp->mRenderThreadCommandBuffers, mDp->mLogicThreadCommandBuffers);
		getRenderContext()->canRenderNextFrame = true;
	}
	void RenderSystem::BeginRenderFrame()
	{
		using namespace Vulkan;
		auto ctx = getRenderContext();
		if (mUseRenderThread) {
			// while (1) {
				//Wait For Logic Frame Ready
			//	while (!getRenderContext()->canRenderNextFrame);
			//	beginRsRenderFrameVk(ctx);


			//	auto nxtImg = waitForNextPresentImage(ctx, (Vulkan::rs_semaphore_vk*)SemaphorePresentImageReady, 0);
			//	{
			//		//Blit main rt things to swapchain
			//		auto curRenderFif = ctx->curRenderFrame % ctx->maxFrameInFlight;
			//		auto& blitPair = mDp->mFiFToBlitToSwapchain[curRenderFif];
			//		if (blitPair.first && blitPair.second) {
			//			cmdBegin(blitPair.second);
			//			Vulkan::transitionImageState((Vulkan::rs_commandbuffer_vk*)blitPair.second, (Vulkan::rs_image_vk*)blitPair.first, ResourceState::TransferSrc);
			//			Vulkan::cmdBlitImage((Vulkan::rs_commandbuffer_vk*)blitPair.second, getRenderContext(), (Vulkan::rs_image_vk*)blitPair.first, getRenderContext()->swapchain->swapchainImgs[nxtImg], Filter::Linear);
			//			cmdEnd(blitPair.second);
			//			blitPair = {};
			//		}
			//	}

			//	for (auto&& cmd : mDp->mRenderThreadCommandBuffers) {
			//		Vulkan::cmdSubmitCmdBuffer(getRenderContext(), (rs_commandbuffer_vk*)cmd.commandBuffer, QueueType_Graphics,  cmd.wait ,  cmd.singal , (Vulkan::rs_fence_vk*)cmd.fence );
			//	}
			//	submitToPresentImage(ctx,, nxtImg, { (Vulkan::rs_semaphore_vk*)mDp->mRenderFinishSemaphore});
			//	ctx->currentSwapchainImage = nxtImg;
			//	if (mDp->mEngineEvent.EngineIdle) {
			//		Vulkan::WaitForDeviceIdel(getRenderContext());
			//		mDp->mEngineEvent.EngineIdle = false;
			//	}
			//	ctx->canRenderNextFrame = false;
			//}
		}
		else {
			mDp->mRenderThreadPresentImage = mDp->mPresentImage;
			beginRsRenderFrameVk(ctx);

			auto nxtImg = waitForNextPresentImage(ctx, (Vulkan::rs_semaphore_vk*)SemaphorePresentImageReady, 0);

			for (auto&& cmd : mDp->mRenderThreadCommandBuffers) {
				Vulkan::cmdSubmitCmdBuffer(getRenderContext(), (rs_commandbuffer_vk*)cmd.commandBuffer, QueueType_Graphics,  cmd.wait , cmd.singal , (Vulkan::rs_fence_vk*)cmd.fence);
			}


			{
				mDp->mRenderThreadCmdBuffer = this->GetCommandBufferInCurRenderThread();
				//Blit main rt things to swapchain
				auto curRenderFif = ctx->curRenderFrame % ctx->maxFrameInFlight;
				cmdBegin(mDp->mRenderThreadCmdBuffer);
				Vulkan::transitionImageState((Vulkan::rs_commandbuffer_vk*)mDp->mRenderThreadCmdBuffer, (Vulkan::rs_image_vk*)mDp->mRenderThreadPresentImage, ResourceState::TransferSrc);
				auto imageSwapchain = getSwapchainImage(nxtImg);
				Vulkan::transitionImageState((Vulkan::rs_commandbuffer_vk*)mDp->mRenderThreadCmdBuffer, (Vulkan::rs_image_vk*)imageSwapchain, ResourceState::TransferDst);
				Vulkan::cmdBlitImage((Vulkan::rs_commandbuffer_vk*)mDp->mRenderThreadCmdBuffer, getRenderContext(), (Vulkan::rs_image_vk*)mDp->mRenderThreadPresentImage, getRenderContext()->swapchain->swapchainImgs[nxtImg], Filter::Linear);
				Vulkan::transitionImageState((Vulkan::rs_commandbuffer_vk*)mDp->mRenderThreadCmdBuffer, (Vulkan::rs_image_vk*)imageSwapchain, ResourceState::Present);
				cmdEnd(mDp->mRenderThreadCmdBuffer);
				//--------//
				//Wait RenderEnd/Image acquire
				//and Signal CanDoPresent
				std::vector<rs_semaphore*> semaphoreToWaitRenderEnd = { mDp->mRenderFinishSemaphore,SemaphorePresentImageReady };
				std::vector<rs_semaphore*> semaphoreToSignalPresentToScreen = { SemaphoreBlitToPresentImageReady };
				//Wait for RenderEnd, swapchain image ready, then semaphore can present to image/RenderEnd
				Vulkan::cmdSubmitCmdBuffer(getRenderContext(), (rs_commandbuffer_vk*)mDp->mRenderThreadCmdBuffer, QueueType_Graphics, semaphoreToWaitRenderEnd, semaphoreToSignalPresentToScreen, (Vulkan::rs_fence_vk*)mDp->mRenderEndFence);
				mDp->mRenderThreadCmdBuffer = nullptr;
				mDp->mRenderThreadPresentImage = nullptr;
			}

			submitToPresentImage(ctx,nxtImg,{ (Vulkan::rs_semaphore_vk*)SemaphoreBlitToPresentImageReady});
			
			ctx->currentSwapchainImage = nxtImg;
			if (mDp->mEngineEvent.EngineIdle) {
				Vulkan::WaitForDeviceIdel(getRenderContext());
				mDp->mEngineEvent.EngineIdle = false;
			}
		}


	}

	void RenderSystem::getWindowSize(int& x, int& y)
	{
		if (this->mWindow) {
			mWindow->getFramebufferSize(x, y);
		}
		else {
			x = -1;
			y = -1;
		}
	}

	void RenderSystem::setCursorEnable(bool enable)
	{
		if (mWindow) {
			mWindow->setCursorEnable(enable);
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

	Render::rs_commandbuffer* RenderSystem::GetCommandBufferInCurRenderThread()
	{
		auto ctx = getRenderContext();
		return ctx->cmdBufferMgr->getCmdBufferCurRenderThread(ctx,QueueType_Graphics, false);
	}

	rs_renderpass* RenderSystem::createRenderPass(const PassDesc& passDescription)
	{
		return Vulkan::createRsRenderPassVk(this->getRenderContext(), passDescription);
	}
	void RenderSystem::destoyRenderPass(rs_renderpass* renderPass)
	{
		if (!renderPass)return;
		Vulkan::rs_renderpass_vk* rpVk = (Vulkan::rs_renderpass_vk*)renderPass;
		Vulkan::destroyRsRenderPassVk(this->getRenderContext(), rpVk);
	}
	void RenderSystem::cmdBeginRenderPass(rs_commandbuffer* cmdbuf, rs_renderpass* pass,std::vector<ClearColor>& clearColor, ClearDepthStencil& clearDs)
	{
		if (isBindlessEnabled()) {
			Vulkan::cmdCollectDrawDataStateToTransit((Vulkan::rs_commandbuffer_vk*)cmdbuf, (Vulkan::rs_bindless_data_vk*)(getGlobalBindlessData()), PipelineType::Graphics, getCurFif());
		}
		if (mDp->mCurrentCameraData) {
			transitDrawdataResourceState(cmdbuf, PipelineType::Graphics, mDp->mCurrentCameraData);
		}
		Vulkan::cmdBeginRenderPass((Vulkan::rs_commandbuffer_vk*)cmdbuf, (Vulkan::rs_renderpass_vk*)pass, clearColor, clearDs);
		cmdbuf->currentRenderPass = pass;
	}
	void RenderSystem::cmdEndRenderPass(rs_commandbuffer* cmdbuf)
	{
		auto curRenderTarget = cmdbuf->currentRenderTarget;
		if (!curRenderTarget)return; //No rt now, also there must be no render pass now
		Vulkan::cmdEndRenderPass((Vulkan::rs_commandbuffer_vk*)cmdbuf);
		for (auto&& img : curRenderTarget->m_attachments) {
			if (img->usage & ImageUsage_PresentSrc) {
				//For render pass contains a present img --> should alawys be the last render pass of the renderflow.
				cmdImageStateTransfer(cmdbuf, img, ResourceState::Present, 0, img->mipLevels, 0, img->arrayLayers);
				break;
			}
		}
	
	}

	void RenderSystem::cmdSetRendertarget(rs_commandbuffer* cmdbuf, rs_rendertarget* rendertarget, const Rect2D& renderArea)
	{
		for (auto img : rendertarget->m_attachments) {
			cmdImageStateTransfer(cmdbuf, img, ResourceState::RenderTarget, 0, img->mipLevels, 0, img->arrayLayers);
		}
		auto depthStencil = rendertarget->m_depthStencilAttachment;
		if (depthStencil) {
			ResourceState toState =
				cmdbuf->currentRenderPass->writeDepth ? ResourceState::DepthStencilWrite : ResourceState::DepthStencilRead;
			cmdImageStateTransfer(cmdbuf, depthStencil, toState, 0, depthStencil->mipLevels, 0, depthStencil->arrayLayers);
		}
		Vulkan::cmdsetRenderTarget(getRenderContext(), (Vulkan::rs_commandbuffer_vk*)cmdbuf, (Vulkan::rs_rendertarget_vk*)rendertarget, renderArea);
	}
	void RenderSystem::cmdSetScissor(rs_commandbuffer* cmdbuf, int framebufferIdx, const Rect2D& rect)
	{
		Vulkan::cmdSetScissor((Vulkan::rs_commandbuffer_vk*)cmdbuf, rect, framebufferIdx);
	}

	void RenderSystem::cmdSetViewport(rs_commandbuffer* cmdbuf, int framebufferIdx, float minDepth, float maxDepth, const Rect2D& rect)
	{
		Vulkan::cmdSetViewport((Vulkan::rs_commandbuffer_vk*)cmdbuf, rect,minDepth,maxDepth,framebufferIdx);
	}

	void RenderSystem::getGlobalViewportZRange(float& nearZ, float& farZ)
	{
		nearZ	= mDp->globalViewPortZNear;
		farZ	= mDp->globalViewPortZFar;
	}

	void RenderSystem::cmdCopyBufferToBuffer(rs_commandbuffer* cmdbuf, rs_buffer* srcBuffer, rs_buffer* dstBuffer, uint32_t srcOffset, uint32_t dstOffset, uint32_t size)
	{
		cmdbuf->hasCommands = true;
		cmdBufferStateTransfer(cmdbuf, srcBuffer, ResourceState::TransferSrc);
		cmdBufferStateTransfer(cmdbuf, dstBuffer, ResourceState::TransferDst);
		Vulkan::cmdCopyBufferToBuffer((Vulkan::rs_commandbuffer_vk*)cmdbuf, getRenderContext(), (Vulkan::rs_buffer_vk*)srcBuffer, (Vulkan::rs_buffer_vk*)dstBuffer, size, srcOffset, dstOffset);
	}

	void RenderSystem::cmdCopyBufferToImage(rs_commandbuffer* cmdbuf, rs_buffer* srcBuffer,rs_image* dstImg,uint32_t srcOffset, int x, int y, int z, int width, int height, int depth, uint32_t baseMip,uint32_t mipCount, int layeroff, int layerSize)
	{
		cmdBufferStateTransfer(cmdbuf, srcBuffer, ResourceState::TransferSrc);
		cmdImageStateTransfer(cmdbuf, dstImg, ResourceState::TransferDst,baseMip,mipCount,layeroff,layerSize);
		for (int i = baseMip; i < baseMip + mipCount; ++i) {
			Vulkan::cmdCopyBufferToImage(
				(Vulkan::rs_commandbuffer_vk*)cmdbuf, getRenderContext(),
				(Vulkan::rs_image_vk*)dstImg,
				(Vulkan::rs_buffer_vk*)srcBuffer,
				srcOffset,
				x, y, z, width, height, depth, i, layeroff, layerSize
			);
		}
	}

	rs_buffer* RenderSystem::createBuffer(void* data, uint32_t size, const BufferDesc& desc)
	{
		rs_buffer* buffer = Vulkan::createRsBufferVk(getRenderContext(), desc);
		if (desc.mappable) {
			//User should notice the frame sync problem.
			//And we just pretend no GPU is using this buffer now.
			buffer->state = ResourceState::HostWrite;

			Vulkan::mapRsBuffer(getRenderContext(), (Vulkan::rs_buffer_vk*)buffer);
		}
		if (data) {
			updateBufferData(buffer, data, size, 0);
		}
		return buffer;
	}


	void RenderSystem::destroyBuffer(rs_buffer* buffer)
	{
		auto rsBuffer = (Vulkan::rs_buffer_vk*)buffer;
		Vulkan::destroyRsBuffer(getRenderContext(), rsBuffer, false);
	}

	void RenderSystem::flushBuffer(rs_buffer* buffer, uint32_t size)
	{
		Vulkan::flushRsBuffer(getRenderContext(), (Vulkan::rs_buffer_vk*)buffer, size);
	}

	rs_compute_pipeline* RenderSystem::createComputePipeline(rs_shader_module* module)
	{
		return Vulkan::createRsComputePipeline(getRenderContext(), (Vulkan::rs_shader_module_vk*)module);
	}

	rs_graphic_pipeline* RenderSystem::createGraphicPipeline(rs_renderpass* renderpass, PipelineDesc& pipelineDescription)
	{
		return Vulkan::createRsGraphicPipeline(getRenderContext(), (Vulkan::rs_renderpass_vk*)renderpass, pipelineDescription);
	}
	void RenderSystem::destroyPipeline(rs_pipeline* pipeline)
	{
		auto plVk = (Vulkan::rs_graphic_pipeline_vk*)pipeline;
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
	rs_drawdata* RenderSystem::getCurCameraDrawData()
	{
		return mDp->mCurrentCameraData;
	}
	rs_drawdata* RenderSystem::createDrawData()
	{
		return Vulkan::createDrawData(getRenderContext());
	}
	rs_image* RenderSystem::createCubemap(void* data, size_t byteSize, ImageFormat format, int x, int y, int z, int arrayLayers, int mipmap)
	{
		ImageDesc desc{};
		desc.width = x;
		desc.height = y;
		desc.depth = z;
		desc.mipLevels = mipmap;
		desc.arrayLayers = arrayLayers * 6;
		desc.format = format;
		desc.usage = ImageUsage::ImageUsage_Sampled | ImageUsage_TransferDst | ImageUsage_Storage;
		if (arrayLayers > 1)
		{
			desc.type = ImageType::VCube_Array;
		}
		else {
			desc.type = ImageType::VCube;
		}

		auto ret = Vulkan::createRsImage(getRenderContext(), desc);

		if (data && byteSize > 0) {
			updateImageData(ret, data, byteSize, 0, 0, 0, x, y, z, 0, 6 * arrayLayers, mipmap);
		}
		return ret;
	}
	rs_image* RenderSystem::createImage2D(void* data, size_t byteSize, ImageFormat format, int x, int y, int z, int layer, int mipmap)
	{
		ImageDesc desc{};
		desc.width = x;
		desc.height = y;
		desc.depth = z;
		desc.mipLevels = mipmap;
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
		desc.usage = ImageUsage::ImageUsage_Sampled | ImageUsage_TransferDst | ImageUsage_Storage;

		auto ret = Vulkan::createRsImage(getRenderContext(), desc);

		if (data) {
			updateImageData(ret, data, byteSize, 0, 0, 0, x, y, z, 0, layer, mipmap);
		}
		return ret;

	}

	void RenderSystem::destroyImage(rs_image* image)
	{
		auto imageVk = (Vulkan::rs_image_vk*)image;
		Vulkan::destroyRsImage(getRenderContext(), imageVk, false);
	}

	size_t RenderSystem::getImageSize(rs_image* img)
	{
		return getRsImageGPUSize(img);
	}

	rs_image* RenderSystem::createRTTexture(RenderTextureFormat format, int x, int y, int z, int layer, int mips, bool needSample, bool uav)
	{
		ImageDesc desc{};
		desc.width = x;
		desc.height = y;
		desc.depth = z;
		desc.mipLevels = mips;
		desc.arrayLayers = layer;

		ImageType type = ImageType::V2D;
		if (z > 1) {
			type = ImageType::V3D;
		}
		else if (layer > 1) {
			type = ImageType::V2D_Array;
		}

		desc.type = type;
		desc.format = fromRtFormatToImageFormat(getRenderContext(),format);

		bool isDepthFMT = false;
		bool isStencilFMT = false;
		if (format == RenderTextureFormat::D24S8 || format == RenderTextureFormat::D32
			|| format == RenderTextureFormat::D32S8) {
		
			isDepthFMT = true;
		}

		if (format == RenderTextureFormat::D24S8
			|| format == RenderTextureFormat::D32S8) {
			isStencilFMT = true;
		}

		if (isDepthFMT || isStencilFMT) {
			desc.usage = ImageUsage_DepthStencilAttachment;
		}
		else {
			desc.usage = ImageUsage::ImageUsage_ColorAttachment;
		}

		if (needSample) {
			desc.usage |= ImageUsage::ImageUsage_Sampled;
		}

		if (uav) {
			desc.usage |= ImageUsage_Storage;
		}

		auto ret =  Vulkan::createRsImage(getRenderContext(), desc);

		return ret;
	}
	rs_image* RenderSystem::createDepthStencilTexture(RenderTextureFormat format, int x, int y,bool needSample)
	{
		ImageDesc desc{};
		desc.width = x;
		desc.height = y;
		desc.depth = 1;
		desc.mipLevels = 1;
		desc.arrayLayers = 1;

		ImageType type = ImageType::V2D;

		desc.type = type;
		desc.format = fromRtFormatToImageFormat(getRenderContext(), format);
		
		desc.usage = ImageUsage::ImageUsage_DepthStencilAttachment;
		if (needSample) {
			desc.usage |= ImageUsage::ImageUsage_Sampled;
		}
		auto ret = Vulkan::createRsImage(getRenderContext(), desc);

		return ret;
	}

	rs_image_view* RenderSystem::getViewFromImage(rs_image* image, const ImageViewKey& viewKey)
	{
		return Vulkan::getRsImageView( this->getRenderContext(), image, viewKey);
	}

	Render::rs_rendertarget* RenderSystem::createRendertargetDetailed(std::vector<rs_image*>& images, std::vector<ImageViewKey>& viewKeys, bool lastDepth)
	{
		auto ctx = getRenderContext();
		return Vulkan::createRsRenderTarget(ctx, (Vulkan::rs_image_vk**)images.data(), (ImageViewKey*)viewKeys.data(), images.size(),lastDepth);
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
		ConstShaderDataManager::instance()->updateSceneDrawData(Scene::getCurrentScene());
		if (getNextRenderFrame() == 0) {
			if (isBindlessEnabled()) {
				bindDefaultResourceToBindless();
			}
		}
		if (isBindlessEnabled()) {
			Vulkan::beginFrameUnbindBindlessResource(getRenderContext(), (Vulkan::rs_bindless_data_vk*)getGlobalBindlessData());
		}
	}

	rs_binding_pos RenderSystem::getBindingPos(const std::string& bindingName, MaterialPass* material)
	{
		auto data = material->getBindingInfoByName(Name(bindingName));
		if (data.has_value()) {
			return (*data).bindingPos;
		}
		return INVALID_BINDING_POS;
	}

	void RenderSystem::updateUniformBufferData(rs_binding_pos binding, void* data, uint32_t size, Pass* pass)
	{	
		this->updateUniformBufferData(binding, data, size, pass->mMaterialPass->getRsPipeline(), pass->mDrawData);
	}
	void RenderSystem::updateUniform(rs_binding_pos binding, int dstArrayElement, rs_buffer* buffer, uint32_t offset, uint32_t size, Pass* pass)
	{
		this->updateUniform(binding, dstArrayElement, buffer, pass->mMaterialPass->getRsPipeline(), pass->mDrawData, offset,size);
	}
	void RenderSystem::updateUniform(rs_binding_pos binding, int dstArrayElement, rs_image* image, Pass* pass)
	{
		this->updateUniform(binding, dstArrayElement, image, pass->mMaterialPass->getRsPipeline(), pass->mDrawData);
	}
	void RenderSystem::updateUniform(rs_binding_pos binding, int dstArrayElement, rs_image_view* view, Pass* pass)
	{
		this->updateUniform(binding, dstArrayElement, view, pass->mMaterialPass->getRsPipeline(), pass->mDrawData);
	}
	void RenderSystem::updateUniform(rs_binding_pos binding, int dstArrayElement, rs_sampler* sampler, Pass* pass)
	{
		this->updateUniform(binding, dstArrayElement, sampler, pass->mMaterialPass->getRsPipeline(), pass->mDrawData);
	}

	rs_binding_pos RenderSystem::getBindingPos(const std::string& bindingName, rs_pipeline* pipeline)
	{
		if(pipeline == nullptr)
			return INVALID_BINDING_POS;
		auto pipelineLayout = pipeline->pipelineLayout;
		if (pipelineLayout == nullptr)
			return INVALID_BINDING_POS;
		rs_binding_pos retPos = INVALID_BINDING_POS;
		for (const auto& binding : pipeline->resources) {
			if (binding.type == ResourceLocationType::BindlessSlot)return INVALID_BINDING_POS;

			if (binding.itemName.isEqual(bindingName)) {
				retPos = binding.bindingPos;
				break;
			}
		}
		return retPos;
	}

	void RenderSystem::updateUniformBufferData(rs_binding_pos binding, void* data, uint32_t size, rs_pipeline* pipeline, rs_drawdata* drawData)
	{
		auto ctx = getRenderContext();
		Vulkan::updateDrawData(ctx, ctx->nextRenderFrame, (Vulkan::rs_graphic_pipeline_vk*)pipeline, (Vulkan::rs_drawdata_vk*)drawData, binding, data, size);
	}

	void RenderSystem::updateUniform(rs_binding_pos binding, int dstArrayElement, rs_buffer* buffer, rs_pipeline* pipeline, rs_drawdata* drawData,uint32_t offset,uint32_t size)
	{
		auto ctx = getRenderContext();
		Vulkan::updateDrawData(ctx, ctx->nextRenderFrame, (Vulkan::rs_graphic_pipeline_vk*)pipeline, (Vulkan::rs_drawdata_vk*)drawData, binding, dstArrayElement, (Vulkan::rs_buffer_vk*)buffer, offset, size);
	}

	void RenderSystem::updateUniform(rs_binding_pos binding, int dstArrayElement, rs_image* image, rs_pipeline* pipelineLayout, rs_drawdata* drawData)
	{
		auto ctx = getRenderContext();
		auto imageToBind = image;
		if (imageToBind == nullptr) {
			imageToBind = geterrorTexture(mDp.get())->getRsImage();
		}
		Vulkan::updateDrawData(ctx, ctx->nextRenderFrame, (Vulkan::rs_graphic_pipeline_vk*)pipelineLayout, (Vulkan::rs_drawdata_vk*)drawData, binding, dstArrayElement, (Vulkan::rs_image_vk*)imageToBind);
	}

	void RenderSystem::updateUniform(rs_binding_pos binding, int dstArrayElement, rs_image* image, ImageViewKey viewkey, rs_pipeline* pipelineLayout, rs_drawdata* drawData)
	{
		auto ctx = getRenderContext();
		auto imageToBind = image;
		if (imageToBind == nullptr) {
			imageToBind = geterrorTexture(mDp.get())->getRsImage();
		}
		rs_image_view* view = getViewFromImage(imageToBind, viewkey);
		if (view == nullptr) {
			auto errImage = geterrorTexture(mDp.get())->getRsImage();
			Vulkan::updateDrawData(ctx, ctx->nextRenderFrame, (Vulkan::rs_graphic_pipeline_vk*)pipelineLayout, (Vulkan::rs_drawdata_vk*)drawData, binding, dstArrayElement, (Vulkan::rs_image_vk*)errImage);
			return;
		}
		Vulkan::updateDrawData(ctx, ctx->nextRenderFrame, (Vulkan::rs_graphic_pipeline_vk*)pipelineLayout, (Vulkan::rs_drawdata_vk*)drawData, binding, dstArrayElement, (Vulkan::rs_image_vk*)imageToBind, view);
	}

	void RenderSystem::updateUniform(rs_binding_pos binding, int dstArrayElement, rs_image_view* view, rs_pipeline* pipelineLayout, rs_drawdata* drawData)
	{
		auto ctx = getRenderContext();
		if (view == nullptr) {
			auto errImage = geterrorTexture(mDp.get())->getRsImage();
			Vulkan::updateDrawData(ctx, ctx->nextRenderFrame, (Vulkan::rs_graphic_pipeline_vk*)pipelineLayout, (Vulkan::rs_drawdata_vk*)drawData, binding, dstArrayElement, (Vulkan::rs_image_vk*)errImage);
			return;
		}
		Vulkan::updateDrawData(ctx, ctx->nextRenderFrame, (Vulkan::rs_graphic_pipeline_vk*)pipelineLayout, (Vulkan::rs_drawdata_vk*)drawData, binding, dstArrayElement, (Vulkan::rs_image_vk*)view->image, view);

	}

	void RenderSystem::updateUniform(rs_binding_pos binding, int dstArrayElement, rs_sampler* sampler, rs_pipeline* pipelineLayout, rs_drawdata* drawData)
	{
		auto ctx = getRenderContext();
		Vulkan::updateDrawData(ctx, ctx->nextRenderFrame, (Vulkan::rs_graphic_pipeline_vk*)pipelineLayout, (Vulkan::rs_drawdata_vk*)drawData, binding,dstArrayElement, (Vulkan::rs_sampler_vk*)sampler);
	}


	void RenderSystem::updateUniform(rs_binding_pos binding,int dstArrayElement, rs_image* image, ImageViewKey viewkey, Pass* pass)
	{
		auto ctx = getRenderContext();
		auto pipeline = (Vulkan::rs_graphic_pipeline_vk*)pass->mMaterialPass->getRsPipeline();
		auto drawData = (Vulkan::rs_drawdata_vk*)pass->mDrawData;
		rs_image_view* view = getViewFromImage(image, viewkey);
		if (view == nullptr) {
			auto errImage = geterrorTexture(mDp.get())->getRsImage();
			Vulkan::updateDrawData(ctx, ctx->nextRenderFrame, pipeline, drawData, binding, dstArrayElement, (Vulkan::rs_image_vk*)errImage);
			return;
		}
		Vulkan::updateDrawData(ctx, ctx->nextRenderFrame, pipeline, drawData, binding, dstArrayElement, (Vulkan::rs_image_vk*)image, view);
	}

	void RenderSystem::updateImageData(rs_image* image, void* data, size_t byteSize, int x, int y, int z, int width, int height, int depth, int layerOffset, int layerSize, int mip)
	{
		auto ctx = getRenderContext();

		assert((image->usage & ImageUsage_TransferDst) && "Image with error usage");

		mDp->mArena->stageBufferUpdate(image, data, byteSize, x, y, z, width, height, depth,mip - 1,1, layerOffset, layerSize);
	}
	void RenderSystem::updateBufferData(rs_buffer* buffer, void* data, size_t byteSize, size_t dstOffset)
	{
		auto ctx = getRenderContext();
		if (buffer->mappedPtr) {
			if (buffer->byteSize - dstOffset < byteSize) {
				assert(0 && "Buffer overflow in updateBufferData! Please check the dstOffset and byteSize!");
				memcpy((uint8_t*)buffer->mappedPtr + dstOffset, data, buffer->byteSize - dstOffset);
			}
			else {
				memcpy((uint8_t*)buffer->mappedPtr + dstOffset, data, buffer->byteSize);
			}
		}
		else {
			mDp->mArena->stageBufferUpdate(buffer, dstOffset, data, byteSize);
		}
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

	void RenderSystem::updateParameters(rs_commandbuffer* cmdBuffer, RenderEntity* entity, Pass* pass)
	{
		entity->updateEntityCommonData();
		if (entity->getMaterial()) {
			entity->getMaterial()->uploadUniform(pass);
		}
		entity->updateUniforms(pass);
		cmdTransferRenderBufferState(cmdBuffer, entity->getRenderInfo());
		auto entityDrawData = pass->mDrawData;
		auto entityCommonDrawData = entity->getEntityCommonDrawData();
		//Transit drawdata resource state to correct state for rendering. Mostly contains 'Image layout transit' or some 'barriers'
		if(entityCommonDrawData)
			transitDrawdataResourceState(cmdBuffer,PipelineType::Graphics, entityCommonDrawData);
		if (entityDrawData)
			transitDrawdataResourceState(cmdBuffer, PipelineType::Graphics, entityDrawData);
	}

	void RenderSystem::dispatchCompute(rs_commandbuffer* cmdBuffer, rs_compute_pipeline* pipeline, rs_drawdata* drawData, int groupX, int groupY, int groupZ)
	{
		if (isBindlessEnabled()) {
			Vulkan::cmdCollectDrawDataStateToTransit((Vulkan::rs_commandbuffer_vk*)cmdBuffer, (Vulkan::rs_bindless_data_vk*)(getGlobalBindlessData()), PipelineType::Compute, getCurFif());
		}
		transitDrawdataResourceState(cmdBuffer, PipelineType::Compute, drawData);
		Vulkan::cmdDispatch((Vulkan::rs_commandbuffer_vk*)cmdBuffer, getRenderContext(),(Vulkan::rs_compute_pipeline_vk*) pipeline, (Vulkan::rs_drawdata_vk*)drawData,(Vulkan::rs_bindless_data_vk*)getGlobalBindlessData(), getCurFif(), groupX, groupY, groupZ);
	}

	void RenderSystem::drawIndexed(rs_commandbuffer* cmdBuffer, RenderEntity* entity, Camera* camera,const Name& passName)
	{
		auto pass = entity->getPass(passName);
		if (!pass)return;
		drawIndexed(cmdBuffer, entity, camera, pass);
	}
	void RenderSystem::drawIndexed(rs_commandbuffer* cmdBuffer, RenderEntity* entity, Camera* camera, Pass* pass)
	{
		cmdBuffer->hasCommands = true;
		auto pipeline = (Vulkan::rs_graphic_pipeline_vk*)pass->mMaterialPass->getRsPipeline();
		auto entityDrawData = (Vulkan::rs_drawdata_vk*)pass->mDrawData;
		auto entityCommonDrawData = entity->getEntityCommonDrawData();
		Vulkan::DrawDataArray drawDataArr{};
		fillDrawDataArray((DrawDataArray*)&drawDataArr, entity, camera, pass);
	
		Vulkan::cmdDrawIndexed((Vulkan::rs_commandbuffer_vk*)cmdBuffer,getRenderContext(), pipeline, entity->getRenderInfo(), drawDataArr, getCurFif(),true);
	}
	void RenderSystem::drawIndexed(rs_commandbuffer* cmdBuffer, rs_graphic_pipeline* pipeline, RenderInfo& info, const DrawDataArray& drawDatas)
	{
		cmdBuffer->hasCommands = true;
		Vulkan::cmdDrawIndexed((Vulkan::rs_commandbuffer_vk*)cmdBuffer, getRenderContext(), (Vulkan::rs_graphic_pipeline_vk*)pipeline, info,(const Vulkan::DrawDataArray&)drawDatas, getCurFif(), true);
	}
	void RenderSystem::transitDrawdataResourceState(rs_commandbuffer* cmdBuffer, PipelineType type, rs_drawdata* drawdata)
	{
		//defered to begin render pass phase
		Vulkan::cmdCollectDrawDataStateToTransit((Vulkan::rs_commandbuffer_vk*)cmdBuffer, (Vulkan::rs_drawdata_vk*)(drawdata), type, getCurFif());
	}
	void RenderSystem::fillDrawDataArray(DrawDataArray* arr, RenderEntity* entity, Camera* camera, Pass* pass)
	{
		auto entityDrawData = pass->mDrawData;
		auto entityCommonDrawData = entity->getEntityCommonDrawData();
		(*arr)[0] = entityCommonDrawData;
		(*arr)[1] = entityDrawData;
		(*arr)[2] = camera!=nullptr ?  camera->getDrawData() : nullptr;
		if (Scene::getCurrentScene()) {
			(*arr)[3] = Scene::getCurrentScene()->getSceneDrawData();
			(*arr)[4] = Scene::getCurrentScene()->getSceneShadowData();
		}

		if (isBindlessEnabled()) {
			(*arr)[5] = (rs_drawdata*)getGlobalBindlessData();
		}
	}

	void RenderSystem::waitForFence(rs_fence* fence,u32 fif)
	{
		Vulkan::waitForRsFence(getRenderContext(), (Vulkan::rs_fence_vk*)fence, -1, fif);
	}

	void RenderSystem::waitForFence(rs_fence* fence)
	{
		Vulkan::waitForRsFence(getRenderContext(), (Vulkan::rs_fence_vk*)fence,-1,getCurFif());
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
		static const std::vector<std::string> ret{ "../shader","shader"
			,"../include/Renderer/GPUShared", "include/Renderer/GPUShared" };
		return ret;
	}

	glm::u32 RenderSystem::getCurRenderFif() const
	{
		return getRenderContext()->RenderFrameFif;
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
		uint32_t NxtImageIdx = (ctx->currentSwapchainImage + 1) % ctx->maxSwapChainImages;
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
		cmdBuffer->recording = true;
	}

	void RenderSystem::cmdEnd(rs_commandbuffer* cmdBuffer)
	{
		cmdBuffer->recording = false;
		Vulkan::cmdEndRecord((Vulkan::rs_commandbuffer_vk*)cmdBuffer);
	}

	rs_semaphore* RenderSystem::createSemaphore(SemaphoreWait waitFlag, ResourceState waitResourceState)
	{
		auto semaphore = Vulkan::createRsSemaphore(getRenderContext());
		semaphore->waitFlag = waitFlag;
		semaphore->waitResourceState = ResourceState::RenderTarget;
		return semaphore;
	}

	void RenderSystem::destroySemaphore(rs_semaphore* semphore)
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

	void RenderSystem::resetFence(rs_fence* fence, uint32_t FiF)
	{
		Vulkan::resetRsFence(getRenderContext(), (Vulkan::rs_fence_vk*)fence, FiF);
	}

	//TODO: remove this.
	void RenderSystem::setCurrentCamera(Camera* camera)
	{
		if (!camera) {
			mDp->mCurrentCameraData = nullptr;
			return;
		}

		if (camera->getCameraActive() == false) {
			assert(0 && "Camera not active");
			mDp->mCurrentCameraData = nullptr;
			return;
		}
		mDp->mCurrentCameraData = camera->getDrawData();
	}

	Camera* RenderSystem::getCurrentCamera()
	{
		return mDp->mCurrentCamera;
	}

	void RenderSystem::setEngineIdle()
	{
		mDp->mEngineEvent.EngineIdle = true;
	}

	void RenderSystem::waitForEngineIdle()
	{
		Vulkan::WaitForDeviceIdel(getRenderContext());
	}

	bool RenderSystem::isRenderTargetCompatibleToRenderPass(rs_renderpass* rp, rs_rendertarget* rt)
	{
		return ((Vulkan::rs_renderpass_vk*)rp)->passHash == ((Vulkan::rs_rendertarget_vk*)rt)->rtPassHash;
	}

	void RenderSystem::excutePendingBufferCopies(rs_commandbuffer* cmdbuf)
	{
		mDp->mArena->executePendingCopies((Vulkan::rs_commandbuffer_vk*)cmdbuf);
	}

	void RenderSystem::cmdBlit(rs_commandbuffer* cmd, TexturePtr from, ImageViewKey fromKey, TexturePtr to, ImageViewKey toKey, Filter filter)
	{
		mDp->mBlitor->cmdBlit(cmd, from, fromKey, to, toKey, filter);
	}

	void RenderSystem::cmdBufferStateTransfer(rs_commandbuffer* cmdbuf, rs_buffer* resource, ResourceState toState)
	{
		cmdbuf->hasCommands = true;
		using namespace Vulkan;
		Vulkan::transitionBufferState(
			(rs_commandbuffer_vk*)cmdbuf,
			(rs_buffer_vk*)resource,
			toState
		);
	}

	void RenderSystem::cmdImageStateTransfer(rs_commandbuffer* cmdbuf, rs_image* resource, ResourceState newState, uint32_t baseMipLevel, uint32_t mipLevelCount, uint32_t baseArrayLayer, uint32_t layerCount)
	{
		cmdbuf->hasCommands = true;
		using namespace Vulkan;
		Vulkan::transitionImageState(
			(rs_commandbuffer_vk*)cmdbuf,
			(rs_image_vk*)resource,
			newState, 
			baseMipLevel,
			mipLevelCount, 
			baseArrayLayer, 
			layerCount
		);
	}

	void RenderSystem::cmdClearRT(rs_commandbuffer* cmdbuf, const TexturePtr& tex, ImageViewKey viewKey, const vec4& color)
	{
		if (mDp->clearRTCompute == nullptr) {
			mDp->clearRTCompute = std::make_unique<ComputeKernel>("../shader/ClearRTCompute.cs", MacroPairs());
		}
		auto view = RenderSystem::instance()->getViewFromImage(tex->getRsImage(), viewKey);
		//is texture a UAV?
		if (view->image->usage & ImageUsage::ImageUsage_Storage)
		{
			float h = view->image->height;
			float w = view->image->width;
			//DO a Compute?
			static Name clearDataName = Name("clearData");
			static Name clearTexName = Name("outClear");
			mDp->clearRTCompute->setParameter(clearDataName, &color, sizeof(color));
			mDp->clearRTCompute->setParameter(clearTexName, tex,viewKey,0);
			mDp->clearRTCompute->dispatch(cmdbuf, std::ceil(w / 8.), std::ceil(h / 8.), 1);
		}
		else {
			if (!Vulkan::cmdClearImage((Vulkan::rs_commandbuffer_vk*)cmdbuf, view, color)) {
				assert(false && "Do a hardware clear requires image with ImageUsage_TransferDst");
				return;
			}

		}
	}

	void RenderSystem::cmdTransferRenderBufferState(rs_commandbuffer* cmdbuf, RenderInfo& renderInfo)
	{
		cmdbuf->hasCommands = true;
		if (renderInfo.indexBuffer != nullptr) {
			cmdBufferStateTransfer(cmdbuf, renderInfo.indexBuffer, ResourceState::IndexBuffer);
		}

		for (auto bufferInfo : renderInfo.bindingBuffers) {
			if (bufferInfo.buffer) {
				cmdBufferStateTransfer(cmdbuf, bufferInfo.buffer, ResourceState::VertexBuffer);
			}
		}
	}


	Render::rs_bindless_data* RenderSystem::createBindlessData(rs_pipeline* pipeline)
	{
		if (!isBindlessEnabled()) {
			return nullptr;
		}

		//Bindless index should alawys in the set0.
		return Vulkan::createBindlessData(getRenderContext(), pipeline,0);
	}

	uint64_t RenderSystem::unbindGlobalBindlessDataBuffer(rs_bindless_data* bindlessData, uint64_t idx)
	{
		return Vulkan::unbindBindlessBuffer(getRenderContext(), (Vulkan::rs_bindless_data_vk*)bindlessData, idx, true);
	}

	uint64_t RenderSystem::updateGlobalBindlessDataBuffer(rs_bindless_data* bindlessData, rs_buffer* buffer)
	{
		if (!buffer)
		{
			Vulkan::updateBindlessData(getRenderContext(), nullptr, nullptr, false);
			return default_bindless_buffer_idx;
		}
		return Vulkan::updateBindlessData(getRenderContext(), (Vulkan::rs_bindless_data_vk*)bindlessData, (Vulkan::rs_buffer_vk*)buffer, false);
	}

	void RenderSystem::markGlobalBindlessDataBuffer(rs_bindless_data* bindlessData, rs_buffer* buffer)
	{
		Vulkan::bindlessDataMarkResource((Vulkan::rs_bindless_data_vk*)bindlessData, buffer, false);
	}

	uint64_t RenderSystem::updateGlobalBindlessDataRWBuffer(rs_bindless_data* bindlessData, rs_buffer* buffer) {
		if (!buffer)
		{
			Vulkan::updateBindlessData(getRenderContext(), nullptr, nullptr, true);
			return default_bindless_buffer_idx;
		}
		return Vulkan::updateBindlessData(getRenderContext(), (Vulkan::rs_bindless_data_vk*)bindlessData, (Vulkan::rs_buffer_vk*)buffer, true);
	}

	void RenderSystem::markGlobalBindlessDataRWBuffer(rs_bindless_data* bindlessData, rs_buffer* buffer)
	{
		Vulkan::bindlessDataMarkResource((Vulkan::rs_bindless_data_vk*)bindlessData, buffer, true);
	}

	uint32_t RenderSystem::unbindGlobalBindlessDataTexture(rs_bindless_data* bindlessData, uint32_t idx)
	{
		if(idx == INVALID_BINDLESS_INDEX)
			return default_bindless_texture_idx;
		return Vulkan::unbindBindlessImage(getRenderContext(), (Vulkan::rs_bindless_data_vk*)bindlessData, idx, false);
	}

	uint32_t RenderSystem::updateGlobalBindlessDataTexture(rs_bindless_data* bindlessData, rs_image_view* img)
	{
		if (!img)return default_bindless_texture_idx;
		return Vulkan::updateBindlessImage(getRenderContext(), (Vulkan::rs_bindless_data_vk*)bindlessData, (rs_image_view*)img, false);
	}

	void RenderSystem::markGlobalBindlessDataTexture(rs_bindless_data* bindlessData, rs_image_view* view)
	{
		Vulkan::bindlessDataMarkResource((Vulkan::rs_bindless_data_vk*)bindlessData, view, false);
	}

	uint32_t RenderSystem::unbindGlobalBindlessDataRWTexture(rs_bindless_data* bindlessData, uint32_t idx)
	{
		if (idx == INVALID_BINDLESS_INDEX)
			return defalut_no_texture_UAV->defaultView->bindlessIndex;
		return Vulkan::unbindBindlessImage(getRenderContext(), (Vulkan::rs_bindless_data_vk*)bindlessData, idx, true);
	}

	uint32_t RenderSystem::updateGlobalBindlessDataRWTexture(rs_bindless_data* bindlessData, rs_image_view* img)
	{
		if (!img)return defalut_no_texture_UAV->defaultView->bindlessIndex;
		return Vulkan::updateBindlessImage(getRenderContext(), (Vulkan::rs_bindless_data_vk*)bindlessData, (rs_image_view*)img, true);
	}

	void RenderSystem::markGlobalBindlessDataRWTexture(rs_bindless_data* bindlessData, rs_image_view* view)
	{
		Vulkan::bindlessDataMarkResource((Vulkan::rs_bindless_data_vk*)bindlessData, view, true);
	}

	uint32_t RenderSystem::unbindGlobalBindlessDataSampler(rs_bindless_data* bindlessData, uint32_t idx)
	{
		if (idx == INVALID_BINDLESS_INDEX)
			return defalut_no_sampler->bindlessIndex;
		return Vulkan::unbindBindlessSampler(getRenderContext(), (Vulkan::rs_bindless_data_vk*)bindlessData, idx);
	}

	uint32_t RenderSystem::updateGlobalBindlessDataSampler(rs_bindless_data* bindlessData, rs_sampler* sampler)
	{
		if(!sampler)
			return default_bindless_sampler_idx;
		return Vulkan::updateBindlessSampler(getRenderContext(), (Vulkan::rs_bindless_data_vk*)bindlessData, (Vulkan::rs_sampler_vk*)sampler);
	}

	bool RenderSystem::isEngineResourceName(const Name& name)
	{
		if (isBindlessEnabled()) {
			static const Name& UAVImageName = Name("GlobalUAVImages");
			static const Name& SamplerName = Name("GlobalSamplers");
			static const Name& TextureName = Name("GlobalTextures");
			return name == UAVImageName || name == SamplerName || name == TextureName;
		}
		
		{

			static const Name& ObjectCommonUBOName = Name("ObjData");
			static const Name& CameraCommonUBOName = Name("CameraCommon");
			static const Name& SceneCommonUBOName = Name("SceneCommon");
			return name == CameraCommonUBOName || name == CameraCommonUBOName || name == SceneCommonUBOName;
		}

		return false;
	}

	void RenderSystem::destroyBindlessData(rs_bindless_data* data)
	{
		//Clear all data ref of bindless
		data->texturesBinding.FreeAll();
		data->storageImagesBinding.FreeAll();
		data->samplersBinding.FreeAll();

		Vulkan::destroyBindlessData(getRenderContext(),(Vulkan::rs_bindless_data_vk*)data);
	}

	void RenderSystem::setGlobalBindlessData(rs_bindless_data* bindlessData)
	{
		if (!isBindlessEnabled())return;

		mDp->globalBindlessData = bindlessData;
	}

	Render::rs_bindless_data* RenderSystem::getGlobalBindlessData()
	{
		if (!isBindlessEnabled())return nullptr;
		if (mDp->globalBindlessData) {
			return mDp->globalBindlessData;
		}
		return nullptr;
	}

	bool RenderSystem::isBindlessEnabled() const
	{
		return Vulkan::isBindlessEnabled();
	}

	void RenderSystem::createPresentRT()
	{
		if (mDp->mPresentRT == nullptr) {
			//Create one 
			ImageDesc desc{};
			desc.format = ImageFormat::RGBA8_UNORM;
			desc.width = mDp->mRenderResolutionX;
			desc.height = mDp->mRenderResolutionY;
			desc.type = ImageType::V2D;
			desc.usage = ImageUsage_TransferSrc | ImageUsage_ColorAttachment;
			mDp->mPresentImage = createRsImage(getRenderContext(), desc);
			mDp->mPresentRT = createRendertarget({ mDp->mPresentImage }, nullptr);
		}
		else {
			assert(false);
		}

	}

	void RenderSystem::destroyPresentRT()
	{
		Vulkan::rs_image_vk* image = (Vulkan::rs_image_vk*)mDp->mPresentImage;
		Vulkan::rs_rendertarget_vk* rt = (Vulkan::rs_rendertarget_vk*)mDp->mPresentRT;
		Vulkan::destroyRsImage(getRenderContext(), image);
		mDp->mPresentImage = nullptr;
		Vulkan::destroyRsRenderTarget(getRenderContext(), rt);
		mDp->mPresentRT = nullptr;
	}

	Render::rs_rendertarget* RenderSystem::getPresentRenderTarget()
	{
		return mDp->mPresentRT;
	}

	Render::rs_image* RenderSystem::getPresentImage()
	{
		return mDp->mPresentImage;
	}

	void RenderSystem::setMainRenderResolution(int x, int y)
	{
		mDp->mRenderResolutionX = x;
		mDp->mRenderResolutionY = y;
	}

	void RenderSystem::blitToSwapchain(rs_image* fromImage)
	{
		assert(fromImage->usage & ImageUsage_TransferSrc);
		mDp->mPresentImage = fromImage;
	}

	Render::rs_semaphore* RenderSystem::getRenderFinishSemaphore()
	{
		return mDp->mRenderFinishSemaphore;
	}

	void RenderSystem::waitLastRenderEnd()
	{
		if (getNextRenderFrame() != 0) {
			waitForFence(mDp->mRenderEndFence, getCurRenderFif());
		}
		resetFence(mDp->mRenderEndFence, getCurRenderFif());
	}

	void RenderSystem::onWindowResize(int x, int y)
	{
		setEngineIdle();
		mDp->mEngineEvent.WindowResize = true;
		mDp->mEngineEvent.IsEngineInBackEnd = true;
	}

	RenderSystem::RenderSystem()
	{
		mDp = std::make_unique< RenderSystemPrivate>();
		
	}
	RenderSystem::~RenderSystem()
	{

	}

	void RenderSystem::bindDefaultResourceToBindless()
	{
		default_bindless_textureuav_idx = updateGlobalBindlessDataRWTexture(getGlobalBindlessData(), defalut_no_texture_UAV->defaultView);
		default_bindless_texture_idx = updateGlobalBindlessDataTexture(getGlobalBindlessData(), defalut_no_texture->defaultView);
		default_bindless_buffer_idx = updateGlobalBindlessDataRWBuffer(getGlobalBindlessData(), defalut_no_buffer);
		default_bindless_sampler_idx = updateGlobalBindlessDataSampler(getGlobalBindlessData(), defalut_no_sampler);
	}

	void RenderSystemPrivate::cleanUpFramesPendingData(uint32_t fif,uint64_t frame)
	{
		mArena->beginFrame(frame);
	}

	TexturePtr geterrorTexture(RenderSystemPrivate* dp)
	{
		if(dp->mErrorRGBTexture == nullptr)
			dp->mErrorRGBTexture = ResourceSystem::instance()->getResource<Texture>(ResourceName::Texture, Name("Builtin::ErrorRGB"));
		return nullptr;
	}

}