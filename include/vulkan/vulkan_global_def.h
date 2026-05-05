#ifndef VULKAN_GLOBAL_DEF_H_
#define VULKAN_GLOBAL_DEF_H_
namespace Render::Vulkan {

	inline bool PartialBindingEnable = false;
	inline bool BufferDeviceAddressEnable = false;
	inline uint32_t BindlessAvailable	= false;
	inline uint32_t BindlessMaxImage	= 1024;
	inline uint32_t BindlessMaxSampler	= 512;
	inline uint32_t BindlessMaxBuffer	= 512;
	inline uint32_t Synchronize2Enable  = false;
}
#endif