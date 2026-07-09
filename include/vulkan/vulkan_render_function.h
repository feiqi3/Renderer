#include "vulkan_render_resource.h"
#include "render_resource_createinfo.h"
#include "render_resource_window.h"
#include "common/CommonMath.h"
#include <vector>
namespace Render::Vulkan {

	//Helpers
	ImageFormat ToImageFormat(VkFormat format);

	VkFormat toVkFormat(ImageFormat fmt);

	VkImageAspectFlags fromEngineAspecttoVkAspect(ViewAspect aspect);
	VkImageAspectFlags toVkAspect(uint32_t usageFlags);

	VkImageViewType toVkImageViewType(ImageType t);
	VkImageType toVkImageType(ImageType t);

	VkSamplerAddressMode toVkAddressMode(AddressMode mode);

	VkFilter toVkFilter(Filter f);

	VkCompareOp toVkCompareOp(CompareOp op);

	VkFrontFace toVkFrontFace(FrontFace face);

	VkSamplerMipmapMode toVkMipmapFilterMode(Filter m);

	VkBufferUsageFlags toVkBufferUsageFlags(uint32_t type);

	VkSampleCountFlagBits toVkSampleCount(SampleCount s);

	VkImageUsageFlags toVkImageUsage(uint32_t u);

	VkBorderColor toVkBorderColor(BorderColor bc);

	VkShaderStageFlags toVkShaderStageFlags(uint16_t stage);

	rs_context_vk* initVulkanBackEnd(const BackEndInitDesc& desc,Window::rs_window* window);
	void createDefaultResources(rs_context_vk* ctx);
	void destroyDefaultResources(rs_context_vk* ctx);
	void queryAllImageFormatCaps(rs_context_vk* ctx);
	void initRenderTextureFormatMapping(rs_context_vk* ctx);
	


	void deinitVulkanBackEnd(rs_context_vk* ctx);
	rs_rendertarget_vk* createRsRenderTarget(rs_context_vk* ctx, rs_image_vk** images, ImageViewKey* imageViewKeys,int imageNum,bool havedepthLast);
	rs_rendertarget_vk* createRsRenderTarget(rs_context_vk* ctx, rs_image_vk** images, int imageNum, rs_image_vk* depthStencil);
	void destroyRsRenderTarget(rs_context_vk* ctx, rs_rendertarget_vk*& rt,bool imm = false);
	
	rs_buffer_vk* createRsBufferVk(rs_context_vk* context,const BufferDesc& desc);
	uint64_t	  getRsBufferDeviceAddress(rs_context_vk* ctx, rs_buffer_vk* buffer);
	void destroyRsBuffer(rs_context_vk* context, rs_buffer_vk*& buffer,bool immediately = false);
	void* mapRsBuffer(rs_context_vk* context, rs_buffer_vk* buffer);
	void  unmapRsBuffer(rs_context_vk* context, rs_buffer_vk* buffer);
	void  flushRsBuffer(rs_context_vk* context, rs_buffer_vk* buffer, uint32_t size);
	uint64_t getOffsetAllocation(decltype(rs_buffer_vk::allocation) allocation);
	VkDeviceMemory getDeviceMemory(decltype(rs_buffer_vk::allocation) allocation);
	bool isRsBufferMappable(rs_context_vk* context, rs_buffer_vk* buffer);

	rs_image_vk* createRsImage(rs_context_vk* context, ImageDesc& desc);
	void destroyRsImage(rs_context_vk* context, rs_image_vk*& image,bool immediately = false);
	size_t getRsImageSize(rs_image_vk* image);
	rs_image_view* getRsImageView(rs_context_vk* ctx, rs_image* image, const ImageViewKey& viewKey);
	rs_image_view* createRsImageView(rs_context_vk* ctx, rs_image* image, ImageType viewType, uint32_t imageUsage, uint16_t baseMip, uint16_t mipCnt, uint16_t baseLayer, uint16_t layerCnt, UAVAccess uav);
	rs_image_view* createRsImageView(rs_context_vk* ctx, rs_image* image, ImageType viewType, ViewAspect aspect, uint16_t baseMip, uint16_t mipCnt, uint16_t baseLayer, uint16_t layerCnt, UAVAccess uav);
	void destroyRsImageView(rs_context_vk* ctx, rs_image_view* view);

	rs_sampler_vk* createRsSampler(rs_context_vk* context,const SamplerDesc& desc);
	void destroyRsSampler(rs_context_vk* context, rs_sampler_vk*& sampler, bool immediately = false);

	rs_shader_module_vk* createRsShader(rs_context_vk* context, ShaderDesc& desc);
	void destroyRsShader(rs_context_vk* context, rs_shader_module_vk*& module);

	rs_semaphore_vk* createRsSemaphore(rs_context_vk* ctx);
	void destroyRsSemaphore(rs_context_vk* ctx, rs_semaphore_vk*& sem);
	rs_fence_vk* createRsFence(rs_context_vk* ctx);
	void destroyRsFence(rs_context_vk* ctx, rs_fence_vk*& fence);
	void resetRsFence(rs_context_vk* ctx, rs_fence_vk* fence, int frameInFlight);
	void resetRsFence(rs_context_vk* ctx, rs_fence_vk* fence);
	void waitForRsFence(rs_context_vk* ctx, rs_fence_vk* fence,uint64_t timeoutNs, int frameInFlight);

	rs_commandbuffer_vk* createRsCommand(rs_context_vk* ctx, const CommandBufferDesc& desc);
	//this function might be called in the render thread, which may have a latency in ctx->maxFrameInFlight frames
	rs_commandbuffer_vk* createRsCommandTargetFrame(rs_context_vk* ctx, const CommandBufferDesc& desc,uint64_t frame);

	uint32_t findQueueFamily(rs_context_vk* ctx, QueueType type);
	void createSwapchain(rs_context_vk* context, ::Render::Window::rs_window* window, rs_swapchain* oldSwapchain = nullptr);
	void destroySwapChain(rs_context_vk* context);
	void createSurface(rs_context_vk* context, ::Render::Window::rs_window* window);
	inline void destroySurface(rs_context_vk* context) {};

	void createVkInstance(rs_context_vk* context);
	void destroyVkInstance(rs_context_vk* context);
	
	void createVkPhysicalDevice(rs_context_vk* context,int chooseOne);

	void createVkQueue(rs_context_vk* context, bool seperateCompute,  bool seperateTransfer);
	void destroyVkQueue(rs_context_vk* context);

	void createVkDevice(rs_context_vk* context);
	void destroyDevice(rs_context_vk* context);
	
	void createDebugUtilsMessengerEXT(rs_context_vk* ctx);
	void destroyDebugUtilsMessengerEXT(rs_context_vk* ctx);
	VkImageSubresourceRange toVkImageSubresourceRange(const ImageViewKey& viewKey);
	std::set<std::string> getExtensionEnableDevice(rs_context_vk* context);
	VkPhysicalDeviceFeatures getExtensionEnablePhysicalDevice(rs_context_vk* context);
	VkPhysicalDeviceFeatures2 getExtensionEnablePhysicalDevice2(rs_context_vk* context);
	VkPhysicalDeviceVulkan13Features getExtensionEnablePhysicalDeviceVk13(rs_context_vk* context);
	VkPhysicalDeviceVulkan12Features getExtensionEnablePhysicalDeviceVk12(rs_context_vk* context);
	VkPhysicalDeviceDescriptorIndexingFeatures getExtensionEnablePhysicalDeviceDescriptorIndexingFeatures(rs_context_vk* context);
	VkPhysicalDeviceBufferDeviceAddressFeatures getExtensionEnablePhysicalDeviceBufferAddressFeatures(rs_context_vk* context);
	std::vector<const char*> getExtensionEnableInstance(rs_context_vk* context);
	std::vector<const char*> getLayerEnableInstance(rs_context_vk* context);
	rs_drawdata_vk* createDrawData(rs_context_vk* context);
	void clearDrawData(rs_context_vk* context, rs_drawdata_vk* drawdata);
	void destroyDrawData(rs_context_vk* context, rs_drawdata_vk* drawdata);
	void updateDrawData(rs_context_vk* context, rs_binding_slot& slot, rs_binding_pos pos, int subscript, void* base, uint32_t dyOffset,uint32_t bufferOffset = 0,uint32_t bufferSize = 0);
	void updateDrawData(rs_context_vk* context, uint64_t frame, rs_graphic_pipeline_vk* pipeline, rs_drawdata_vk* drawdata, rs_binding_pos pos, void* data, size_t size);
	void updateDrawData(rs_context_vk* context, uint64_t frame, rs_graphic_pipeline_vk* pipeline, rs_drawdata_vk* drawdata, rs_binding_pos pos, int subscript, rs_image_vk* vk);
	void updateDrawData(rs_context_vk* context, uint64_t frame, rs_graphic_pipeline_vk* pipeline, rs_drawdata_vk* drawdata, rs_binding_pos pos, int subscript, rs_image_vk* vk, rs_image_view* view);
	void updateDrawData(rs_context_vk* context, uint64_t frame, rs_graphic_pipeline_vk* pipeline, rs_drawdata_vk* drawdata, rs_binding_pos pos, int subscript, rs_sampler_vk* vk);
	void updateDrawData(rs_context_vk* context, uint64_t frame, rs_graphic_pipeline_vk* pipeline, rs_drawdata_vk* drawdata, rs_binding_pos pos, int subscript, rs_buffer_vk* vk,uint32_t offset,uint32_t size);
	void bindlessDataMarkResource(rs_bindless_data_vk* bindlessData, rs_image_view* view	,bool isUAV);
	void bindlessDataMarkResource(rs_bindless_data_vk* bindlessData, rs_buffer* buffer		, bool isUAV);
	rs_buffer_vk* createStageBufferTemp(rs_context_vk* context, uint64_t size);
	//-------------------------------------------------------------------------------------//     
	void cmdsetRenderTarget(rs_context_vk* ctx, rs_commandbuffer_vk* cmd, rs_rendertarget_vk* rt, const Rect2D& renderArea);
	void cmdBeginRenderPass(rs_commandbuffer_vk* cb, rs_renderpass_vk* renderpass, const std::vector<ClearColor>& color, const ClearDepthStencil& clearDs);
	void cmdEndRenderPass(rs_commandbuffer_vk* cb);
	void cmdSetViewport(rs_commandbuffer_vk* cb,const Rect2D& rect,float minDepth,float maxDepth,uint32_t idx);
	void cmdSetScissor(rs_commandbuffer_vk* cb,const Rect2D& rect, uint32_t idx);

	void cmdDispatch(rs_commandbuffer_vk* cb, rs_context_vk* ctx, rs_compute_pipeline_vk* pipeline, rs_drawdata_vk* drawData, rs_bindless_data_vk* bindless, uint32_t curFIF,int x,int y,int z);
	void cmdDrawIndexed(rs_commandbuffer_vk* cb, rs_context_vk* ctx, rs_graphic_pipeline_vk* pipeline, const RenderInfo& info, DrawDataArray drawDatas, uint32_t curFif, bool isInstanced = false,bool wireFrame = false);
	void cmdDrawIndexed(rs_commandbuffer_vk* cb, rs_context_vk* ctx,rs_graphic_pipeline_vk* pipeline,const RenderInfo& info,rs_drawdata_vk* drawData,uint32_t curFif,bool isInstanced = false, bool wireFrame = false);
	void cmdBindDrawData(rs_commandbuffer_vk* cb, rs_context_vk* ctx, rs_pipeline_layout_vk* layout, rs_drawdata_vk* drawData, uint32_t curFif,QueueType bindPoint = QueueType::QueueType_Graphics);
	void cmdCopyBufferToBuffer(rs_commandbuffer_vk* cb, rs_context_vk* context, rs_buffer_vk* bufferSrc,rs_buffer_vk* bufferdst,uint64_t size,uint64_t srcOffset, uint64_t dstOffset);
	
	void cmdCollectDrawDataStateToTransit(rs_commandbuffer_vk* cb, rs_bindless_data_vk* drawData, PipelineType pipelineType, uint32_t curFif);
	void cmdCollectDrawDataStateToTransit(rs_commandbuffer_vk* cb, rs_drawdata_vk* drawData, PipelineType pipelineType, uint32_t curFif);
	void cmdTransitPendingResource(rs_commandbuffer_vk* cb,bool compute);
	//Decrepted
	void cmdCopyBufferToImage(rs_commandbuffer_vk* cb, rs_context_vk* context, rs_image_vk* image, rs_buffer_vk* buffer, uint32_t srcOffset, int x, int y, int z, int width, int height, int depth, uint32_t mip, int layeroff, int layerSize);
	void cmdCopyImageToBuffer(rs_commandbuffer_vk* cb, rs_context_vk* context, rs_image_vk* image, rs_buffer_vk* buffer, uint32_t bufferOffset, int x, int y, int z, int width, int height, int depth, uint32_t mip, int layeroff, int layerSize);
	void cmdSubmitCmdBuffer(rs_context_vk* ctx, rs_commandbuffer_vk* cb,QueueType queue,std::vector<rs_semaphore*> imageAvailableWaitSemaphores,std::vector<rs_semaphore*> renderFinishSignalSemphores,rs_fence_vk* fence);
	void cmdBeginMark(rs_commandbuffer_vk* cb, const char* mark, float r, float g, float b, float a);
	void cmdEndMark(rs_commandbuffer_vk* cb);
	void cmdInsertMark(rs_commandbuffer_vk* cb, const char* mark, float r, float g, float b, float a);
	void cmdBeginRecord(rs_commandbuffer_vk* cb);
	void cmdEndRecord(rs_commandbuffer_vk* cb);
	void cmdSubmitOneShotAndWait(rs_context_vk* ctx, rs_commandbuffer_vk* cb);
	void cmdBlitImage(rs_commandbuffer_vk* cb, rs_context_vk* ctx, rs_image_vk* srcImage, rs_image_vk* dstImage, Filter filter);
	bool cmdClearImage(rs_commandbuffer_vk* cb, rs_image_view* image, const vec4& color);
	//-------------------------------------------------------------------------------------//     
	
	//Bindless related. 
	// **Still need to init it's binding pos inside when call it.**
	void				 cmdBindBindlessData(rs_context_vk* ctx, rs_commandbuffer_vk* cmd,rs_pipeline_layout_vk* pipelineLayout, rs_bindless_data_vk* bindlessData);

	rs_bindless_data_vk* createBindlessData(rs_context_vk* ctx,rs_pipeline* pipeline,int setIdx);
	void				 destroyBindlessData(rs_context_vk* ctx, rs_bindless_data_vk* data);
	
	//For DBA, we actually do not need bindless buffer anymore....Just get the real  address on gpu, but since this is only for vulkan,,,,
	uint64_t				updateBindlessData(rs_context_vk* ctx, rs_bindless_data_vk* bindlessData, rs_buffer_vk* buffer, bool uav /*UAV OR SRV ?(Uniform buffer VS Storage Buffer)*/);
	uint32_t				updateBindlessImage(rs_context_vk* ctx, rs_bindless_data_vk* bindlessData, rs_image_view* view, bool uav /*texture2D vs image2D*/);

	uint32_t				unbindBindlessImage(rs_context_vk* ctx, rs_bindless_data_vk* bindlessData, uint32_t index, bool uav /*texture2D vs image2D*/);
	uint64_t				unbindBindlessBuffer(rs_context_vk* ctx, rs_bindless_data_vk* bindlessData, uint64_t index, bool uav /*texture2D vs image2D*/);

	uint32_t				updateBindlessSampler(rs_context_vk* ctx, rs_bindless_data_vk* bindlessData, rs_sampler_vk* sampler);
	uint32_t				unbindBindlessSampler(rs_context_vk* ctx, rs_bindless_data_vk* bindlessData, uint32_t index);
	void					beginFrameUnbindBindlessResource(rs_context_vk* ctx, rs_bindless_data_vk* bindlessData);
	//-------------------------------------------------------------------------------------//     
	uint64_t beginRsFrameVk(rs_context_vk* ctx);
	uint64_t beginRsRenderFrameVk(rs_context_vk* ctx);
	uint64_t endRsFrameVk(rs_context_vk* ctx);
	uint64_t waitForNextPresentImage(rs_context_vk* ctx,rs_semaphore_vk* imageAvailableSignalSemaphore,rs_fence_vk* fenceToSignal);
	void submitToPresentImage(rs_context_vk* ctx, uint32_t presentImgIdx, std::vector<rs_semaphore_vk*> canPresentToScreen);
	void WaitForDeviceIdel(rs_context_vk* ctx);
	//-------------------------------------------------------------------------------------//     
	bool isBindlessEnabled();


}