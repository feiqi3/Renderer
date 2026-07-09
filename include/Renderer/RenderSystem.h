#ifndef RENDER_SYSTEM_H_
#define RENDER_SYSTEM_H_
#include <memory>
#include "render_resource_createinfo.h"
#include "common/NoCopyable.h"
#include "Renderer/RenderEntity.h"
#include "Renderer/MaterialVarient.h"
#include "Common/Name.h"
#include <render_resource.h>
#include "Renderer/ResourceFwd.h"
namespace Render{
	class RenderQueue;
	class Camera;

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
		void getWindowSize(int& x,int& y);
		void setCursorEnable(bool enable);
		class RenderPassManager* getRenderPassManager()const;
		RenderPass* getRenderPass(const Name& pass);
		rs_commandbuffer* GetCommandBufferCurFrameCurThread();
		rs_commandbuffer* GetCommandBufferInCurRenderThread();
		rs_renderpass* createRenderPass(const PassDesc& passDescription);
		void destoyRenderPass(rs_renderpass* renderPass);
		void cmdBeginRenderPass(rs_commandbuffer* cmdbuf,rs_renderpass* pass, std::vector<ClearColor>& clearColor, ClearDepthStencil& clearDs);
		void cmdEndRenderPass(rs_commandbuffer* cmdbuf);
		void cmdSetRendertarget(rs_commandbuffer* cmdbuf, rs_rendertarget* rendertarget, const Rect2D& renderArea = {});

		void cmdSetScissor(rs_commandbuffer* cmdbuf, int framebufferIdx, const Rect2D& rect);
		void cmdSetViewport(rs_commandbuffer* cmdbuf, int framebufferIdx, float minDepth, float maxDepth,const Rect2D& rect);

		void getGlobalViewportZRange(float& nearZ,	float& farZ);
		//Gain a better performance
		void cmdCopyBufferToBuffer(rs_commandbuffer* cmdbuf, rs_buffer* srcBuffer, rs_buffer* dstBuffer, uint32_t srcOffset, uint32_t dstOffset, uint32_t size);
		void cmdCopyBufferToImage(rs_commandbuffer* cmdbuf, rs_buffer* srcBuffer, rs_image* dstImg, uint32_t srcOffset, int x, int y, int z, int width, int height, int depth, uint32_t baseMip, uint32_t mipCount, int layeroff, int layerSize);
		rs_buffer* createBuffer(void* data, uint32_t size, const BufferDesc& desc);
		void destroyBuffer(rs_buffer* buffer);

		void flushBuffer(rs_buffer* buffer, uint32_t size);

		rs_compute_pipeline* createComputePipeline(rs_shader_module* module);
		rs_graphic_pipeline* createGraphicPipeline(rs_renderpass* renderpass,PipelineDesc& pipelineDescription);
		void destroyPipeline(rs_pipeline* pipeline);
		rs_sampler* createSampler(const SamplerDesc& desc);
		void destroyRsSampler(rs_sampler* sampler);
		rs_drawdata* createDrawData();
		void clearDrawData(rs_drawdata* data);
		void destroyDrawData(rs_drawdata* data);
		rs_drawdata* getCurCameraDrawData();

		rs_image* createCubemap(void* data, size_t byteSize, ImageFormat format, int x, int y, int z, int arrayLayers, int mipmap);
		rs_image* createImage2D(void* data, size_t byteSize, ImageFormat format, int x, int y, int z, int layer, int mipmap);
		void destroyImage(rs_image* image);
		//It's quite hard to calculate Device size of a texture...   
		size_t getImageSize(rs_image* img);
		rs_image* createRTTexture(RenderTextureFormat format, int x, int y, int z, int layer,int mipLevel, bool needSample, bool uav = false);
		rs_image* createDepthStencilTexture(RenderTextureFormat format, int x, int y, bool needSample);
		rs_image_view* getViewFromImage(rs_image* image,const ImageViewKey& viewKey);
		//Create more rendertarget with more detailed control settings
		rs_rendertarget* createRendertargetDetailed( std::vector<rs_image*>& images, std::vector<ImageViewKey>& viewKeys,bool lastDepth);
		
		//Create more rendertarget with default settings(which is fair enough)
		rs_rendertarget* createRendertarget(const std::vector<rs_image*>images, rs_image* dsTex);
		void destroyRenderTarget(rs_rendertarget* rt);
		void beginFrame();

		//In Logic Frame only
		rs_binding_pos getBindingPos(const std::string& bindingName, MaterialPass* material);
		void updateUniformBufferData(rs_binding_pos binding, void* data, uint32_t size, Pass* pass);
		void updateUniform(rs_binding_pos binding, int dstArrayElement, rs_buffer* buffer,uint32_t offset,uint32_t size, Pass* pass);
		void updateUniform(rs_binding_pos binding, int dstArrayElement, rs_image* image, Pass* pass);
		void updateUniform(rs_binding_pos binding, int dstArrayElement, rs_image_view* view, Pass* pass);
		void updateUniform(rs_binding_pos binding, int dstArrayElement, rs_image* image,ImageViewKey viewkey, Pass* pass);
		void updateUniform(rs_binding_pos binding, int dstArrayElement, rs_sampler* sampler, Pass* pass);

		rs_binding_pos getBindingPos(const std::string& bindingName, rs_pipeline* pipelineLayout);
		void updateUniformBufferData(rs_binding_pos binding, void* data, uint32_t size, rs_pipeline* pipelineupdateUniformBufferData, rs_drawdata* drawData);
		void updateUniform(rs_binding_pos binding, int dstArrayElement, rs_buffer* buffer, rs_pipeline* pipelineLayout, rs_drawdata* drawData,uint32_t offset = 0,uint32_t size = 0);
		void updateUniform(rs_binding_pos binding, int dstArrayElement, rs_image* image, rs_pipeline* pipelineLayout, rs_drawdata* drawData);
		void updateUniform(rs_binding_pos binding, int dstArrayElement, rs_image* image, ImageViewKey viewkey, rs_pipeline* pipelineLayout, rs_drawdata* drawData);
		void updateUniform(rs_binding_pos binding, int dstArrayElement, rs_image_view* view, rs_pipeline* pipelineLayout, rs_drawdata* drawData);
		void updateUniform(rs_binding_pos binding, int dstArrayElement, rs_sampler* sampler, rs_pipeline* pipelineLayout, rs_drawdata* drawData);


		void updateImageData(rs_image* image, void* data, size_t byteSize, int x, int y, int z, int width, int height, int depth, int layerOffset, int layerSize, int mip);
		void updateBufferData(rs_buffer* buffer,void* data,size_t byteSize,size_t dstOffset);
		void submitCmdBuffer(rs_commandbuffer* cmdBuffer,const std::vector<rs_semaphore*>& wait, const std::vector<rs_semaphore*>& singal,rs_fence* fence);
		void updateParameters(rs_commandbuffer* cmdBuffer, RenderEntity* entity, Pass* pass);

		void dispatchCompute(rs_commandbuffer* cmdBuffer, rs_compute_pipeline* pipeline, rs_drawdata* drawData, int groupX, int groupY, int groupZ);

		void drawIndexed(rs_commandbuffer* cmdBuffer, RenderEntity* entity, const Name& passName);
		void drawIndexed(rs_commandbuffer* cmdBuffer, RenderEntity* entity, Pass* pass);
		
		void drawIndexed(rs_commandbuffer* cmdBuffer, rs_graphic_pipeline* pipeline, RenderInfo& info, const DrawDataArray& drawDatas);
		void transitDrawdataResourceState(rs_commandbuffer* cmdBuffer, PipelineType type , rs_drawdata* drawdata);
		void fillDrawDataArray(DrawDataArray* arr, RenderEntity* entity, Pass* pass);
		void waitForFence(rs_fence* fence);
		void waitForFence(rs_fence* fence, u32 frameInFlight);
		void clearRenderEntity(RenderEntity* entity);

		ShaderIncFindFunc getShaderIncludeSearchFunc()const;
		const std::vector<std::string>& getShaderIncludeSearchDir()const;

		u32		 getCurRenderFif()const;
		uint64_t getNextRenderFrame()const;
		uint32_t getCurFif()const;

		rs_image* getSwapchainImage(uint32_t idx);
		rs_rendertarget* getNextSwapchainRendertarget();

		void initSwapchainRT();
		void deinitSwapchainRT();

		void cmdBegin(rs_commandbuffer* cmdBuffer);
		void cmdEnd(rs_commandbuffer* cmdBuffer);

		rs_semaphore* createSemaphore(SemaphoreWait waitFlag = SemaphoreWait::CurRenderFrame, ResourceState waitResourceState = ResourceState::RenderTarget);
		//Set to -1 to wait for current frame
		void destroySemaphore(rs_semaphore* semphore);

		//Not signaled state.
		rs_fence* createFence();
		void destroyFence(rs_fence* fence);
		void resetFence(rs_fence* fence);
		void resetFence(rs_fence* fence,uint32_t FiF);

		void setCurrentCamera(Camera* camera);
		Camera* getCurrentCamera();

		void setSignalCanRenderToPresentImageSemaphore(rs_semaphore*semaphores) {
			SemaphorePresentImageReady = semaphores;
		}
		void setSignalCanPresentToPresentImageSemaphore(rs_semaphore* semaphores) {
			SemaphoreBlitToPresentImageReady = semaphores;
		}

		void setEngineIdle();
		void waitForEngineIdle();

		bool isRenderTargetCompatibleToRenderPass(rs_renderpass* rp, rs_rendertarget* rt);

		void excutePendingBufferCopies(rs_commandbuffer* cmdbuf);
		void cmdBlit(rs_commandbuffer* cmd, TexturePtr from, ImageViewKey fromKey, TexturePtr to, ImageViewKey toKey, Filter filter);
		void cmdBufferStateTransfer(rs_commandbuffer* cmdbuf,rs_buffer* resource, ResourceState toState);
		void cmdImageStateTransfer(rs_commandbuffer* cmdbuf,rs_image* resource,
			ResourceState newState,
			uint32_t baseMipLevel,
			uint32_t mipLevelCount,
			uint32_t baseArrayLayer,
			uint32_t layerCount
		);
		void cmdClearRT(rs_commandbuffer* cmdbuf,const TexturePtr& tex,ImageViewKey viewKey, const vec4& color);
		void cmdTransferRenderBufferState(rs_commandbuffer* cmdbuf, RenderInfo& renderInfo);
		rs_bindless_data* createBindlessData(rs_pipeline* pipeline);

		uint64_t unbindGlobalBindlessDataBuffer(rs_bindless_data* bindlessData, uint64_t idx);
		uint64_t updateGlobalBindlessDataBuffer(rs_bindless_data* bindlessData,rs_buffer* buffer);
		void	 markGlobalBindlessDataBuffer(rs_bindless_data* bindlessData, rs_buffer* buffer);
		uint64_t updateGlobalBindlessDataRWBuffer(rs_bindless_data* bindlessData, rs_buffer* buffer);
		void	 markGlobalBindlessDataRWBuffer(rs_bindless_data* bindlessData, rs_buffer* buffer);

		uint32_t unbindGlobalBindlessDataTexture(rs_bindless_data* bindlessData, uint32_t idx);
		uint32_t updateGlobalBindlessDataTexture(rs_bindless_data* bindlessData, rs_image_view* img);
		void	 markGlobalBindlessDataTexture(rs_bindless_data* bindlessData, rs_image_view* view);

		uint32_t unbindGlobalBindlessDataRWTexture(rs_bindless_data* bindlessData, uint32_t idx);
		uint32_t updateGlobalBindlessDataRWTexture(rs_bindless_data* bindlessData, rs_image_view* img);
		void	 markGlobalBindlessDataRWTexture(rs_bindless_data* bindlessData, rs_image_view* view);

		uint32_t unbindGlobalBindlessDataSampler(rs_bindless_data* bindlessData, uint32_t idx);
		uint32_t updateGlobalBindlessDataSampler(rs_bindless_data* bindlessData, rs_sampler* sampler);
		bool	 isEngineResourceName(const Name& name);

		void destroyBindlessData(rs_bindless_data* data);
		void			  setGlobalBindlessData(rs_bindless_data * bindlessData);
		rs_bindless_data* getGlobalBindlessData();
		bool isBindlessEnabled()const;
		void			 createPresentRT();
		void			 destroyPresentRT();
		rs_rendertarget* getPresentRenderTarget();
		rs_image*		 getPresentImage();
		void			 setMainRenderResolution(int x, int y);
		//Actully we do a defer blit inside cause we can only get 
		void			 blitToSwapchain(rs_image* fromImage);
		rs_semaphore*    getRenderFinishSemaphore();
		void			 waitLastRenderEnd();
	public:
		void onWindowResize(int x,int y);
	private:
		RenderSystem();
		~RenderSystem();

		void bindDefaultResourceToBindless();
	private:
		uint64_t mCurLogicFrameInFlight = 0;
		uint64_t currentLogicFrame = 0;
		Vulkan::rs_context_vk* mBackEndContext;
		Window::rs_window* mWindow;
		std::unique_ptr<RenderSystemPrivate> mDp;
		bool mUseRenderThread = false;

		rs_semaphore* SemaphorePresentImageReady ;
		rs_semaphore* SemaphoreBlitToPresentImageReady ;
		
	private: 
		static RenderSystem* sRenderSystem;
	};
}

#endif