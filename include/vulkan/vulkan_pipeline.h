#ifndef VULKAN_PIPELINE_H
#define VULKAN_PIPELINE_H
#include "vulkan_render_resource.h"
#include "vulkan_descriptor_set.h"
#include <vector>
namespace Render::Vulkan {
	
	rs_pipeline_layout_vk* createPipelineLayout(rs_context_vk* context, const std::vector<std::pair<uint16_t, rs_descriptorset_layout_vk*>>& setLayouts, const std::vector<VkPushConstantRange>& pushConstants);

	rs_renderpass_vk* createRenderPass(
		rs_context_vk* ctx,
		const PassDesc & rpDesc
	);

}


#endif