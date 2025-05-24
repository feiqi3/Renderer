#ifndef VULKAN_PIPELINE_H
#define VULKAN_PIPELINE_H
#include "vulkan_render_resource.h"
#include "vulkan_descriptor_set.h"
#include <vector>
namespace Render::Vulkan {

	rs_pipeline_layout_vk* createPipelineLayout(rs_context_vk* context, const std::vector<std::pair<uint16_t, rs_descriptorset_layout_vk*>>& setLayouts, const std::vector<VkPushConstantRange>& pushConstants);

	rs_pipeline_vk* createPipeline(rs_context_vk* ctx, rs_renderpass_vk* renderPass, const PipelineDesc& desc);

	rs_pipeline_layout_vk* createPipelineLayoutVk(rs_context_vk* ctx, const std::vector< DescritporSetInfo>& descriptorInfos);
	void destroyPipelineLayout(rs_context_vk* ctx, rs_pipeline_layout_vk*& layout);

	VkShaderStageFlags toVkShaderStageFlags(uint32_t stage);
	VkShaderStageFlagBits toVkShaderStageBit(ShaderStage stage);

	rs_renderpass_vk* createRenderPass(
		rs_context_vk* ctx,
		const PassDesc& rpDesc
	);

	VkPrimitiveTopology toVkTopology(Topology topo);
	VkPolygonMode toVkFillMode(FillMode mode);
	VkCullModeFlags toVkCullMode(CullMode mode);
	VkStencilOp toVkStencilOp(StencilOp op);

	VkBlendFactor toVkBlendFactor(BlendFactor bf);

	VkBlendOp toVkBlendOp(BlendOp op);
}

#endif