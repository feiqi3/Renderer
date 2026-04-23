#ifndef VULKAN_SHADER_MODULE_H
#define VULKAN_SHADER_MODULE_H
#include <vector>
#include "vulkan/vulkan_descriptor_set.h"

namespace Render::Vulkan {

	struct DescritporSetInfo {
		int setIdx = -1;
		rs_vk_descriporset_layout_hash layoutHash;
	};

	struct PipelineLayoutInfo {
		std::vector<DescritporSetInfo> setInfo;
		std::vector<BindlessInfo> bindlessInfo;
	};
	bool assembleBindlessInfo(const std::vector<BindlessInfo>& info, std::vector<BindlessInfo>& out);
	std::vector< DescritporSetInfo> assembleDescriptorSetInfo(const std::vector<rs_descriptor>& descritpors);

	PipelineLayoutInfo getPipelineShaderInfo(rs_shader_module_vk** shaders, size_t num);

}

#endif