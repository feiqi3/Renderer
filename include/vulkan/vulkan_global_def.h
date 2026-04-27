#ifndef VULKAN_GLOBAL_DEF_H_
#define VULKAN_GLOBAL_DEF_H_
namespace Render::Vulkan {
	//This could be wrong when doing something across multi device...But who cares
	inline struct rs_image_vk* defalut_no_texture = nullptr;
	inline struct rs_image_vk* defalut_no_texture_UAV = nullptr;
	inline struct rs_sampler_vk* defalut_no_sampler = nullptr;
	inline struct rs_buffer_vk* defalut_no_buffer = nullptr;
	inline struct rs_buffer_vk* defalut_no_buffer_UAV = nullptr;
	inline bool PartialBindingEnable = false;
	inline bool BufferDeviceAddressEnable = false;
	inline uint32_t BindlessAvailable	= false;
	inline uint32_t BindlessMaxImage	= 1024;
	inline uint32_t BindlessMaxSampler	= 512;
	inline uint32_t BindlessMaxBuffer	= 512;
	inline uint32_t Synchronize2Enable  = false;
}
#endif