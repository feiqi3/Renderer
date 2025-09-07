#ifndef VULKAN_SHADER_MODULE_H
#define VULKAN_SHADER_MODULE_H
#include <vector>
#include "vulkan/vulkan_descriptor_set.h"

namespace Render::Vulkan {

	struct DescritporSetInfo {
		int setIdx = -1;
		rs_vk_descriporset_layout_hash layoutHash;
	};

	std::vector< DescritporSetInfo> assembleDescriptorSetInfo(const std::vector<rs_descriptor>& descritpors);

	std::vector< DescritporSetInfo> getPipelineShaderInfo(rs_shader_module_vk** shaders, size_t num);

}

#endif