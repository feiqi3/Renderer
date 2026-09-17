#include "vulkan/vulkan_global_def.h"
namespace Render::Vulkan {

	bool PartialBindingEnable = false;
	bool BufferDeviceAddressEnable = false;
	bool BindlessAvailable = false;
	uint32_t BindlessMaxImage = 1024;
	uint32_t BindlessMaxSampler = 512;
	uint32_t BindlessMaxBuffer = 512;
	uint32_t Synchronize2Enable = false;
	bool		DeviceFault = false;
	bool		ShaderDebugPrint = false;
	bool		ShaderDrawMeta = false;
}