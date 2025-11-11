#ifndef RENDER_SYSTEM_H_
#define RENDER_SYSTEM_H_
#include <memory>
#include "render_resource_createinfo.h"
#include "common/NoCopyable.h"
#include "Renderer/RenderEntity.h"
#include "Renderer/MaterialVarient.h"
#include <render_resource.h>
namespace Render{
	namespace Window {
		class rs_window;
	}
	namespace Vulkan {
		struct rs_context_vk;
	}

	class RenderSystemPrivate;
	class RenderSystem:public Common::NonCopyable {
	public:
		static void createRenderSystem(const BackEndInitDesc& backEndDesc, Window::rs_window* window);
		static void destroyRenderSystem();
		static RenderSystem* instance();
		void EndLogicFrame();
		void BeginRenderFrame();
	public:
		inline Vulkan::rs_context_vk* getRenderContext()const {
			return (Vulkan::rs_context_vk*)mBackEndContext;
		}

		class RenderPassManager* getRenderPassManager()const;
		RenderPass* getRenderPass(const std::string& pass);
		rs_commandbuffer* GetCommandBufferCurFrameCurThread();
		rs_renderpass* createRenderPass(rs_rendertarget* renderTarget,const PassDesc& passDescription);
		void destoyRenderPass(rs_renderpass* renderPass);
		void cmdBeginRenderPass(rs_commandbuffer* cmdbuf,rs_renderpass* pass, std::vector<ClearColor>& clearColor, ClearDepthStencil& clearDs);
		void cmdEndRenderPass(rs_commandbuffer* cmdbuf);

		void cmdSetScissor(rs_commandbuffer* cmdbuf, int framebufferIdx, const Rect2D& rect);
		void cmdSetViewport(rs_commandbuffer* cmdbuf, int framebufferIdx, float minDepth, float maxDepth,const Rect2D& rect);

		rs_buffer* createBuffer(void* data, uint32_t size, const BufferDesc& desc);
		void destroyBuffer(rs_buffer* buffer);

		void updateBuffer(rs_buffer* buffer,void* data, uint32_t size,uint32_t offset,bool imm = false);

		rs_pipeline* createRenderPipeline(rs_renderpass* renderpass,PipelineDesc& pipelineDescription);
		void destroyRenderPipeline(rs_pipeline* pipeline);
		rs_sampler* createSampler(const SamplerDesc& desc);
		void destroyRsSampler(rs_sampler* sampler);
		rs_drawdata* createDrawData();
		void clearDrawData(rs_drawdata* data);
		void destroyDrawData(rs_drawdata* data);

		rs_image* createImage2D(void* data, size_t byteSize, ImageFormat format, int x, int y, int z, int layer, int mipmap);
		rs_image* createRTTexture(ImageFormat format, int x, int y, int z, int layer,bool needSample);
		rs_image* createDepthStencilTexture(ImageFormat format, int x, int y, bool needSample);
		rs_rendertarget* createRendertarget(const std::vector<rs_image*>images,rs_image* dsTex);
		void destroyRenderTarget(rs_rendertarget* rt);
		void beginFrame();

		//In Logic Frame only
		rs_binding_pos getBindingPos(const std::string& bindingName, Material* material);
		rs_binding_pos getBindingPos(const std::string& bindingName,MaterialTemplate* matTemplate,const std::string& passName);
		void updateUniformBufferData(rs_binding_pos binding, void* data, uint32_t size, Pass* pass);
		void updateUniform(rs_binding_pos binding, rs_buffer* buffer, Pass* pass);
		void updateUniform(rs_binding_pos binding, rs_image* image, Pass* pass);
		void updateUniform(rs_binding_pos binding, rs_sampler* sampler, Pass* pass);
		void updateImageData(rs_image* image, void* data, size_t byteSize, int x, int y, int z, int width, int height, int depth, int layerOffset, int layerSize, int mip);
		void updateBufferData(rs_buffer* buffer,void* data,size_t byteSize,size_t dstOffset);
		void* placeFramePendingData(void* data, uint32_t size);
		void submitCmdBuffer(rs_commandbuffer* cmdBuffer,const std::vector<rs_semaphore*>& wait, const std::vector<rs_semaphore*>& singal,rs_fence* fence);
		void drawIndexed(rs_commandbuffer* cmdBuffer, RenderEntity* entity, const std::string& passName);
		void drawIndexed(rs_commandbuffer* cmdBuffer, RenderEntity* entity, Pass* pass);
		void waitForFence(rs_fence* fence);
		void clearRenderEntity(RenderEntity* entity);

		uint64_t getNextRenderFrame()const;
		uint32_t getCurFif()const;

		rs_image* getSwapchainImage(uint32_t idx);
		rs_rendertarget* getNextSwapchainRendertarget();

		void initSwapchainRT();
		void deinitSwapchainRT();

		void cmdBegin(rs_commandbuffer* cmdBuffer);
		void cmdEnd(rs_commandbuffer* cmdBuffer);

		rs_semaphore* createSemphore();
		void destroySemphore(rs_semaphore* semphore);

		rs_fence* createFence();
		void destroyFence(rs_fence* fence);
		void resetFence(rs_fence* fence);

		void setSignalCanRenderToPresentImageSemaphore(rs_semaphore*semaphores) {
			SignalCanRenderToPresentImageSemaphore = semaphores;
		}
		void setSignalCanPresentToPresentImageSemaphore(rs_semaphore* semaphores) {
			SignalCanPresentToPresentImageSemaphore = semaphores;
		}

		void setRenderFence(rs_fence* fence) {
			FenceToWaitForRender = fence;
		}

	private:
		RenderSystem();
		~RenderSystem();

	private:
		uint64_t mCurLogicFrameInFlight = 0;
		uint64_t currentLogicFrame = 0;
		Vulkan::rs_context_vk* mBackEndContext;
		Window::rs_window* mWindow;
		std::unique_ptr<RenderSystemPrivate> mDp;
		bool mUseRenderThread = false;

		rs_semaphore* SignalCanRenderToPresentImageSemaphore ;
		rs_semaphore* SignalCanPresentToPresentImageSemaphore ;
		rs_fence* FenceToWaitForRender = 0;
	private: 
		static RenderSystem* sRenderSystem;
	};
}

#endif