#include "vulkan_render_resource.h"
#include "render_resource_createinfo.h"

namespace Render::Vulkan {

	//Helpers
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

	uint32_t findQueueFamily(rs_context_vk* ctx, QueueType type);

	rs_buffer_vk* createRsBuffer(rs_context_vk* context, BufferDesc& desc);
	void* mapRsBuffer(rs_context_vk* context, rs_buffer_vk* buffer);
	void  unmapRsBuffer(rs_context_vk* context, rs_buffer_vk* buffer);
	bool isRsBufferMappable(rs_context_vk* context, rs_buffer_vk* buffer);

	rs_image_vk* createRsImage(rs_context_vk* context, ImageDesc& desc);

	rs_sampler_vk* createRsSampler(rs_context_vk* context, SamplerDesc& desc);

	rs_shader_module_vk* createRsShader(rs_context_vk* context, ShaderDesc& desc);

	rs_commandbuffer_vk* createRsCommand(rs_context_vk* ctx, const CommandBufferDesc& desc);

}