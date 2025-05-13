#include "vulkan_render_resource.h"
#include "render_resource_createinfo.h"

namespace Render::Vulkan {

	//Helpers
	VkFormat toVkFormat(ImageFormat fmt);

	VkImageViewType toVkImageViewType(ImageViewType t);

	VkSamplerAddressMode toVkAddressMode(AddressMode mode);

	VkFilter toVkFilter(Filter f);

	VkSamplerMipmapMode toVkMipmapFilterMode(Filter m);

	VkBufferUsageFlags toVkBufferUsageFlags(uint32_t type);

	rs_buffer_vk* createRsBuffer(rs_context_vk* context, BufferDesc& desc);

	void* mapRsBuffer(rs_context_vk* context, rs_buffer_vk* buffer);
	void  unmapRsBuffer(rs_context_vk* context, rs_buffer_vk* buffer);

	bool isRsBufferMappable(rs_context_vk* context, rs_buffer_vk* buffer);
}