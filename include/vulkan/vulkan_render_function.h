#include "vulkan_render_resource.h"
#include "render_resource_createinfo.h"
#include "render_resource_window.h"

#include <vector>
namespace Render::Vulkan {

	//Helpers
	ImageFormat ToImageFormat(VkFormat format);

	VkFormat toVkFormat(ImageFormat fmt);

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

	void initVulkanBackEnd(BackEndInitDesc& desc,Window::rs_window* window);

	rs_rendertarget_vk* createRsRenderTarget(rs_context_vk* ctx,const std::vector<rs_image_vk>& images, rs_image_vk* depthStencil);
	rs_buffer_vk* createRsBuffer(rs_context_vk* context, BufferDesc& desc);
	void destroyRsBuffer(rs_context_vk* context, rs_buffer_vk*& buffer,bool immediately = false);
	void* mapRsBuffer(rs_context_vk* context, rs_buffer_vk* buffer);
	void  unmapRsBuffer(rs_context_vk* context, rs_buffer_vk* buffer);
	bool isRsBufferMappable(rs_context_vk* context, rs_buffer_vk* buffer);

	rs_image_vk* createRsImage(rs_context_vk* context, ImageDesc& desc);
	void destroyRsImage(rs_context_vk* context, rs_image_vk*& image,bool immediately = false);

	rs_sampler_vk* createRsSampler(rs_context_vk* context, SamplerDesc& desc);
	void destroyRsSampler(rs_context_vk* context, rs_sampler_vk*& sampler, bool immediately = false);

	rs_shader_module_vk* createRsShader(rs_context_vk* context, ShaderDesc& desc);
	void destroyRsShader(rs_context_vk* context, rs_shader_module_vk*& module);

	rs_semaphore_vk* createRsSemaphore(rs_context_vk* ctx);
	void destroyRsSemaphore(rs_context_vk* ctx, rs_semaphore_vk*& sem);
	rs_fence_vk* createRsFence(rs_context_vk* ctx);
	void destroyRsFence(rs_context_vk* ctx, rs_fence_vk*& fence);
	void resetRsFence(rs_context_vk* ctx, rs_fence_vk* fence);
	void waitForRsFence(rs_context_vk* ctx, rs_fence_vk* fence,uint64_t timeoutNs);

	rs_commandbuffer_vk* createRsCommand(rs_context_vk* ctx, const CommandBufferDesc& desc);

	uint32_t findQueueFamily(rs_context_vk* ctx, QueueType type);
	void createSwapchain(rs_context_vk* context, ::Render::Window::rs_window* window, rs_swapchain* oldSwapchain = nullptr);
	void createSurface(rs_context_vk* context, ::Render::Window::rs_window* window);
	void createVkInstance(rs_context_vk* context);
	void createVkPhysicalDevice(rs_context_vk* context,int chooseOne);
	void createVkQueue(rs_context_vk* context, bool seperateCompute,  bool seperateTransfer);
	void createVkDevice(rs_context_vk* context);

	void createDebugUtilsMessengerEXT(rs_context_vk* ctx);

	std::vector<const char*> getExtensionEnableDevice(rs_context_vk* context);
	VkPhysicalDeviceFeatures getExtensionEnablePhysicalDevice(rs_context_vk* context);
	VkPhysicalDeviceFeatures2 getExtensionEnablePhysicalDevice2(rs_context_vk* context);
	std::vector<const char*> getExtensionEnableInstance(rs_context_vk* context);
	std::vector<const char*> getLayerEnableInstance(rs_context_vk* context);

	rs_buffer_vk* createStageBufferTemp(rs_context_vk* context,uint64_t size);
	//-------------------------------------------------------------------------------------//     
	void cmdBeginRenderPass(rs_commandbuffer_vk* cb,rs_renderpass_vk* renderpass, std::vector<ClearColor>& color,ClearDepthStencil& clearDs);
	void cmdEndRenderPass(rs_commandbuffer_vk* cb);
	void cmdSetViewport(rs_commandbuffer_vk* cb,Rect2D& rect);
	void cmdSetScissor(rs_commandbuffer_vk* cb, Rect2D& rect);
	void cmdDrawIndexed(rs_commandbuffer_vk* cb,const RenderInfo& info,bool isInstanced = false);
	void cmdUpdateBufferData(rs_commandbuffer_vk* cb, rs_context_vk* context,rs_buffer_vk* buffer, void* data, uint64_t size,uint64_t dstOffset = 0);
	void cmdUpdateImage(rs_commandbuffer_vk* cb, rs_context_vk* context,rs_image_vk* image, void* data, uint64_t size, int mip, int layer);
	void cmdImageLayoutTo(rs_commandbuffer_vk* cb, rs_image_vk* image, VkImageLayout newlayout,int mip,int layer,uint32_t aspect);
	void cmdSubmitCmdBuffer(rs_context_vk* ctx, rs_commandbuffer_vk* cb,QueueType queue,std::vector<rs_semaphore_vk*> waitSemaphores,std::vector<rs_semaphore_vk*> signalSemaphores,rs_fence_vk* fence);
	//-------------------------------------------------------------------------------------//     
}