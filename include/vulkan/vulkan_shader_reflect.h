#ifndef VULKAN_SHADER_REFLECT_H
#define VULKAN_SHADER_REFLECT_H
#include "vulkan_render_resource.h"
#include "vulkan_shader_module.h"
#include "vulkan_descriptor_set.h"
#include <vector>
namespace Render::Vulkan {
	const int SHADER_COMPILE_VULKAN_VERSION = 2;
	rs_shader_module_vk* compileShader(rs_context_vk* ctx, const ShaderCompileDesc& desc);
	void reflectShader(rs_shader_module_vk* shader,uint32_t* spirv_code,uint64_t codeSize);
}

#endif //VULKAN_SHADER_REFLECT_H