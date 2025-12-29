#ifndef VULKAN_PIPELINE_H
#define VULKAN_PIPELINE_H
#include "vulkan_render_resource.h"
#include "vulkan_descriptor_set.h"
#include "vulkan_shader_module.h"
#include <vector>
namespace Render::Vulkan {

	rs_pipeline_layout_vk* createRsPipelineLayout(rs_context_vk* context, const std::vector<std::pair<uint16_t, rs_descriptorset_layout_vk*>>& setLayouts, const std::vector<VkPushConstantRange>& pushConstants);
	rs_pipeline_layout_vk* createRsPipelineLayout(rs_context_vk* ctx, const std::vector< DescritporSetInfo>& descriptorInfos);

	rs_pipeline_vk* createRsPipeline(rs_context_vk* ctx, rs_renderpass_vk* renderPass, const PipelineDesc& desc);
	void destroyRsPipeline(rs_context_vk* ctx, rs_pipeline_vk*& pipeline,bool immediately = false);

	void destroyRsPipelineLayout(rs_context_vk* ctx, rs_pipeline_layout_vk*& layout);
	VkImageLayout pickLayout(uint32_t usage, Render::StorageOp op);
	VkShaderStageFlags toVkShaderStageFlags(uint32_t stage);
	VkShaderStageFlagBits toVkShaderStageBit(ShaderStage stage);

	uint64_t CalcRenderTargetPassHash(rs_context_vk* ctx, const rs_rendertarget_vk* rt);

	VkRenderPass getOrCreateRenderPassCacheVk(
		rs_context_vk* ctx,
		rs_renderpass_vk* renderpass,
		rs_rendertarget_vk* rt
	);

	VkRenderPass createRenderPassCacheVk(
		rs_context_vk* ctx,
		rs_renderpass_vk* renderpass,
		rs_rendertarget_vk* rt
	);

	rs_renderpass_vk* createRsRenderPassVk(
		rs_context_vk* ctx,
		const PassDesc& rpDesc
	);
	void destroyRsRenderPassVk(rs_context_vk* ctx, rs_renderpass_vk*& renderpass, bool immediately = false);

	VkPrimitiveTopology toVkTopology(Topology topo);
	VkPolygonMode toVkFillMode(FillMode mode);
	VkCullModeFlags toVkCullMode(CullMode mode);
	VkStencilOp toVkStencilOp(StencilOp op);

	VkBlendFactor toVkBlendFactor(BlendFactor bf);

	VkBlendOp toVkBlendOp(BlendOp op);
}

#endif