#ifndef VULKAN_GLOBAL_DEF_H_
#define VULKAN_GLOBAL_DEF_H_
#include <cstdint>
namespace Render::Vulkan {

	extern bool PartialBindingEnable;
	extern bool BufferDeviceAddressEnable;
	extern bool BindlessAvailable;
	extern uint32_t BindlessMaxImage;
	extern uint32_t BindlessMaxSampler;
	extern uint32_t BindlessMaxBuffer;
	extern uint32_t Synchronize2Enable;
	extern bool		DeviceFault;
	extern bool		ShaderDebugPrint;
	extern bool		ShaderDrawMeta;
}
#endif