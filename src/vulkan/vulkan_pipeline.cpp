#include "vulkan/vulkan_pipeline.h"
#include "vulkan/vulkan_descriptor_set.h"
#include "vulkan/vulkan_render_resource.h"
#include "vulkan_render_function.cpp"

#include "vulkan/vulkan_shader_module.h"
namespace Render::Vulkan {

    namespace {
        VkImageLayout pickLayout(uint32_t usage, Render::StorageOp op) {
            using namespace Render;

            if (usage & ImageUsage_ColorAttachment) {
                return (op == StorageOp::DontCare)
                    ? VK_IMAGE_LAYOUT_UNDEFINED
                    : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            }
            // 深度/模板附件
            if (usage & ImageUsage_DepthStencilAttachment) {
                return (op == StorageOp::DontCare)
                    ? VK_IMAGE_LAYOUT_UNDEFINED
                    : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }
            return VK_IMAGE_LAYOUT_UNDEFINED;
        }

    }


    VkShaderStageFlags toVkShaderStageFlags(uint32_t stage)
    {
        VkShaderStageFlags flags = 0;

        // 图形管线阶段
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::Vertex))
            flags |= VK_SHADER_STAGE_VERTEX_BIT;               // :contentReference[oaicite:0]{index=0}
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::TessControl))
            flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT; // :contentReference[oaicite:1]{index=1}
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::TessEvaluation))
            flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT; // :contentReference[oaicite:2]{index=2}
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::Geometry))
            flags |= VK_SHADER_STAGE_GEOMETRY_BIT;             // :contentReference[oaicite:3]{index=3}
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::Fragment))
            flags |= VK_SHADER_STAGE_FRAGMENT_BIT;             // :contentReference[oaicite:4]{index=4}

        // 计算管线
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::Compute))
            flags |= VK_SHADER_STAGE_COMPUTE_BIT;              // :contentReference[oaicite:5]{index=5}

        // 射线追踪阶段（KHR 扩展）
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::RayGen))
            flags |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;           // :contentReference[oaicite:6]{index=6}
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::AnyHit))
            flags |= VK_SHADER_STAGE_ANY_HIT_BIT_KHR;          // :contentReference[oaicite:7]{index=7}
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::ClosestHit))
            flags |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;      // :contentReference[oaicite:8]{index=8}
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::Miss))
            flags |= VK_SHADER_STAGE_MISS_BIT_KHR;             // :contentReference[oaicite:9]{index=9}
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::Intersection))
            flags |= VK_SHADER_STAGE_INTERSECTION_BIT_KHR;     // :contentReference[oaicite:10]{index=10}
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::Callable))
            flags |= VK_SHADER_STAGE_CALLABLE_BIT_KHR;         // :contentReference[oaicite:11]{index=11}

        // Task/mesh 阶段（EXT_mesh_shader 扩展）
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::Task))
            flags |= VK_SHADER_STAGE_TASK_BIT_EXT;             // :contentReference[oaicite:12]{index=12}
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::Mesh))
            flags |= VK_SHADER_STAGE_MESH_BIT_EXT;             // :contentReference[oaicite:13]{index=13}

        return flags;
    }

    VkShaderStageFlagBits toVkShaderStageBit(ShaderStage stage)
    {
        VkShaderStageFlags flags = 0;

        switch (stage)
        {
        case Render::ShaderStage::None:
            return VK_SHADER_STAGE_VERTEX_BIT;
            break;
        case Render::ShaderStage::Vertex:
            return VK_SHADER_STAGE_VERTEX_BIT;
            break;
        case Render::ShaderStage::Fragment:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
            break;
        case Render::ShaderStage::Compute:
            return VK_SHADER_STAGE_COMPUTE_BIT;
            break;
        case Render::ShaderStage::TessControl:
            return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            break;
        case Render::ShaderStage::TessEvaluation:
            return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            break;
        case Render::ShaderStage::Geometry:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
            break;
        case Render::ShaderStage::RayGen:
            return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
            break;
        case Render::ShaderStage::AnyHit:
            return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
            break;
        case Render::ShaderStage::ClosestHit:
            return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
            break;
        case Render::ShaderStage::Miss:
            return VK_SHADER_STAGE_MISS_BIT_KHR;
            break;
        case Render::ShaderStage::Intersection:
            return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
            break;
        case Render::ShaderStage::Callable:
            return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
            break;
        case Render::ShaderStage::Task:
            return VK_SHADER_STAGE_TASK_BIT_EXT;
            break;
        case Render::ShaderStage::Mesh:
            return VK_SHADER_STAGE_MESH_BIT_EXT;
            break;
        default:
            return VK_SHADER_STAGE_VERTEX_BIT;
            break;
        }
    }

    rs_pipeline_layout_vk* createPipelineLayout(rs_context_vk* context, const std::vector<std::pair<uint16_t, rs_descriptorset_layout_vk*>>& setLayouts, const std::vector<VkPushConstantRange>& pushConstants)
    {
        
        uint32_t maxSets = 0;
        
        for (auto&& [binding, setLayout] : setLayouts) {
            setLayout->accquir();
            maxSets = std::max(uint32_t(binding), maxSets);
        }
        std::vector<VkDescriptorSetLayout> setlayout_vks;
        std::fill(setlayout_vks.begin(), setlayout_vks.end(), context->descriptorSetMgr->getEmptyDescriptorSetLayout(context));

        for (auto&& [binding, setLayout] : setLayouts) {
            setlayout_vks[binding] = (VkDescriptorSetLayout)setLayout->native;
        }

        VkPipelineLayoutCreateInfo plcInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        plcInfo.setLayoutCount = setlayout_vks.size();
        plcInfo.pSetLayouts = setlayout_vks.data();
        plcInfo.pushConstantRangeCount = pushConstants.size();
        plcInfo.pPushConstantRanges = pushConstants.data();
        VkPipelineLayout res;
        VK_CHECK(vkCreatePipelineLayout(context->device, &plcInfo, 0, &res), {
            return nullptr;
            }
        );
           
        auto layout = new rs_pipeline_layout_vk();
        layout->setLayouts = setLayouts;
        layout->pushConstants = pushConstants;
        layout->native = res;
        return layout;
    }
    rs_renderpass_vk* createRenderPass(rs_context_vk* ctx, PassDesc const& rpDesc)
    {
        std::vector<VkAttachmentDescription> vkAttachments;
        vkAttachments.reserve(rpDesc.attachments.size());
        for (auto const& a : rpDesc.attachments) {
            VkAttachmentDescription ad;
            ad.format = toVkFormat(a.format);
            ad.samples = toVkSampleCount(a.samples);
            // loadOps
            ad.loadOp = (a.loadOp == StorageOp::Clear)
                ? VK_ATTACHMENT_LOAD_OP_CLEAR
                : (a.loadOp == StorageOp::DontCare)
                ? VK_ATTACHMENT_LOAD_OP_DONT_CARE
                : VK_ATTACHMENT_LOAD_OP_LOAD;
            // storeOps
            ad.storeOp = (a.storeOp == StorageOp::Clear) ? VK_ATTACHMENT_STORE_OP_NONE
                : ((a.storeOp == StorageOp::Cached) ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE);
            // stencil 默认不使用
            ad.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            ad.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            // 布局
            ad.initialLayout = pickLayout(a.usage, a.loadOp);
            ad.finalLayout = pickLayout(a.usage, a.storeOp);
            vkAttachments.push_back(ad);
        }
        // VkSubpassDescription (+ AttachmentReference)
        VkSubpassDescription sd{};
        sd.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        // 假设 RenderPassDesc.subpasses.size() == 1
        // 准备颜色 AttachmentReference 数组
        std::vector<VkAttachmentReference> colorRefs;
        colorRefs.reserve(rpDesc.attachments.size());
        bool hasDepthRef = false;
        VkAttachmentReference deepRef{};
        for (auto i = 0; i < rpDesc.attachments.size(); ++i) {
            int idx = i;
            auto& att = rpDesc.attachments[i];
            VkAttachmentReference ref{};
            ref.attachment = i;
            if (att.usage & ImageUsage::ImageUsage_ColorAttachment) {
                ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            }
            else if (att.usage & ImageUsage::ImageUsage_DepthStencilAttachment) {
                hasDepthRef = true;
                deepRef.attachment = i;
                deepRef.layout = rpDesc.writeDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                continue;
            }
            colorRefs.push_back(ref);
        }
        
        VkRenderPassCreateInfo rpci{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
        rpci.attachmentCount = uint32_t(vkAttachments.size());
        rpci.pAttachments = vkAttachments.data();
        rpci.subpassCount = 1;
        rpci.pSubpasses = &sd;
        rpci.dependencyCount = 0;
        rpci.pDependencies = nullptr;
        VkRenderPass rdpass;
        VK_CHECK(vkCreateRenderPass(ctx->device, &rpci, 0, &rdpass), {
            return nullptr;
            }
        );

        auto* rp = new rs_renderpass_vk();
        rp->passDesc = rpDesc;  // 复制描述
        rp->native = rdpass;

        return rp;
    }

    VkPrimitiveTopology toVkTopology(Topology topo)
    {
        switch (topo)
        {
        case Render::Topology::TriangleList:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            break;
        case Render::Topology::TriangleStrip:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            break;
        case Render::Topology::TriangleFAn:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
            break;
        case Render::Topology::Point:
            return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            break;
        case Render::Topology::LineList:
            return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            break;
        case Render::Topology::LineStrip:
            return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
            break;
        default:
            break;
        }
    }

    VkPolygonMode toVkFillMode(FillMode mode)
    {
        switch (mode)
        {
        case Render::FillMode::Fill:
            return VK_POLYGON_MODE_FILL;
            break;
        case Render::FillMode::Line:
            return VK_POLYGON_MODE_LINE;
            break;
        case Render::FillMode::Point:
            return VK_POLYGON_MODE_POINT;
            break;
        default:
            return VK_POLYGON_MODE_FILL;
            break;
        }
    }

    VkCullModeFlags toVkCullMode(CullMode mode)
    {
        switch (mode)
        {
        case Render::CullMode::None:
            return VK_CULL_MODE_NONE;
            break;
        case Render::CullMode::Front:
            return VK_CULL_MODE_FRONT_BIT;
            break;
        case Render::CullMode::Back:
            return VK_CULL_MODE_BACK_BIT;
            break;
        case Render::CullMode::FrontAndBack:
            return VK_CULL_MODE_FRONT_AND_BACK;
            break;
        default:
            return VK_CULL_MODE_NONE;
            break;
        }
    }

    VkStencilOp toVkStencilOp(StencilOp op)
    {
        switch (op) {
        case StencilOp::Keep:              return VK_STENCIL_OP_KEEP;
        case StencilOp::Zero:              return VK_STENCIL_OP_ZERO;
        case StencilOp::Replace:           return VK_STENCIL_OP_REPLACE;
        case StencilOp::IncrementAndClamp: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
        case StencilOp::DecrementAndClamp: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
        case StencilOp::Invert:            return VK_STENCIL_OP_INVERT;
        case StencilOp::IncrementAndWrap:  return VK_STENCIL_OP_INCREMENT_AND_WRAP;
        case StencilOp::DecrementAndWrap:  return VK_STENCIL_OP_DECREMENT_AND_WRAP;
        default:                           return VK_STENCIL_OP_KEEP;
        }
    }

    VkBlendFactor toVkBlendFactor(BlendFactor bf)
    {
        switch (bf) {
        case BlendFactor::Zero:               return VK_BLEND_FACTOR_ZERO;                   // :contentReference[oaicite:0]{index=0}
        case BlendFactor::One:                return VK_BLEND_FACTOR_ONE;                    // :contentReference[oaicite:1]{index=1}
        case BlendFactor::SrcColor:           return VK_BLEND_FACTOR_SRC_COLOR;              // :contentReference[oaicite:2]{index=2}
        case BlendFactor::OneMinusSrcColor:   return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;    // :contentReference[oaicite:3]{index=3}
        case BlendFactor::DstColor:           return VK_BLEND_FACTOR_DST_COLOR;              // :contentReference[oaicite:4]{index=4}
        case BlendFactor::OneMinusDstColor:   return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;    // :contentReference[oaicite:5]{index=5}
        case BlendFactor::SrcAlpha:           return VK_BLEND_FACTOR_SRC_ALPHA;              // :contentReference[oaicite:6]{index=6}
        case BlendFactor::OneMinusSrcAlpha:   return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;    // :contentReference[oaicite:7]{index=7}
        case BlendFactor::DstAlpha:           return VK_BLEND_FACTOR_DST_ALPHA;              // :contentReference[oaicite:8]{index=8}
        case BlendFactor::OneMinusDstAlpha:   return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;    // :contentReference[oaicite:9]{index=9}
        }
        // Fallback
        return VK_BLEND_FACTOR_ONE;
    }

    VkBlendOp toVkBlendOp(BlendOp op)
    {
        switch (op) {
        case BlendOp::Add:               return VK_BLEND_OP_ADD;               // :contentReference[oaicite:0]{index=0}
        case BlendOp::Subtract:          return VK_BLEND_OP_SUBTRACT;          // :contentReference[oaicite:1]{index=1}
        case BlendOp::ReverseSubtract:   return VK_BLEND_OP_REVERSE_SUBTRACT;  // :contentReference[oaicite:2]{index=2}
        case BlendOp::Min:               return VK_BLEND_OP_MIN;               // :contentReference[oaicite:3]{index=3}
        case BlendOp::Max:               return VK_BLEND_OP_MAX;               // :contentReference[oaicite:4]{index=4}
        }
        // 默认回退
        return VK_BLEND_OP_ADD;
    }

    rs_pipeline_vk* createPipeline(rs_context_vk* ctx, rs_renderpass_vk* renderPass, const PipelineDesc& desc)
    {
        VkGraphicsPipelineCreateInfo ci{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

        std::vector<VkPipelineShaderStageCreateInfo> shaders;
        for (auto&& i : desc.shaders) {
            VkPipelineShaderStageCreateInfo psCI{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
            psCI.module = (VkShaderModule)i->native;
            psCI.pName = i->entryPoint.c_str();
            psCI.stage = toVkShaderStageBit(i->shaderStage);
            psCI.module = (VkShaderModule)i->native;
            psCI.pSpecializationInfo = 0;
            shaders.push_back(psCI);
        }

        ci.stageCount = desc.shaders.size();
        ci.pStages = shaders.data();

        VkPipelineVertexInputStateCreateInfo vtxInputState{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vtxInputState.vertexBindingDescriptionCount = desc.vertexInputDesc.bindings.size();
        std::vector<VkVertexInputBindingDescription> vtxBinding;
        for (auto&& i : desc.vertexInputDesc.bindings) {
            VkVertexInputBindingDescription bd{};
            bd.binding = i.binding;
            bd.stride = i.stride;
            bd.inputRate = i.perInstance == true ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
            vtxBinding.push_back(bd);
        }

        vtxInputState.pVertexBindingDescriptions = vtxBinding.data();
        vtxInputState.vertexAttributeDescriptionCount = vtxBinding.size();

        std::vector<VkVertexInputAttributeDescription> vtxInputDesc{};
        for (auto&& i : desc.vertexInputDesc.attributes) {
            VkVertexInputAttributeDescription attr{};
            attr.    location = i.location;
            attr.    binding = i.binding;
            attr.    format = toVkFormat(i.format);
            attr.offset = i.offset;
            vtxInputDesc.push_back(attr);
        }
        vtxInputState.pVertexAttributeDescriptions = vtxInputDesc.data();
        vtxInputState.vertexAttributeDescriptionCount = vtxInputDesc.size();

        ci.pVertexInputState = &vtxInputState;
        VkPipelineInputAssemblyStateCreateInfo assCi{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        assCi.                        topology = toVkTopology(desc.renderState.topology);
        assCi.                        primitiveRestartEnable = false;

        ci. pInputAssemblyState = &assCi;

        //TODO:  Tessellation
        ci. pTessellationState = 0;

        //TODO: MultiViewport
        VkPipelineViewportStateCreateInfo vpCI{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        VkViewport vp{};
        std::vector<VkViewport> vps(ctx->viewportCount);
        std::vector<VkRect2D> scisses(ctx->scissorCount);
        vpCI.viewportCount = ctx->viewportCount;
        vpCI. pViewports = vps.data();
        vpCI.scissorCount = 1;
        vpCI.pScissors = scisses.data();


        ci. pViewportState = &vpCI;

        VkPipelineRasterizationStateCreateInfo rsCi{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rsCi.                                   depthClampEnable = VK_FALSE;
        rsCi.                                   rasterizerDiscardEnable = VK_FALSE;
        rsCi.                              polygonMode = toVkFillMode(desc.renderState.fillMode);
        rsCi.                            cullMode = toVkCullMode(desc.renderState.cullMode);
        rsCi.frontFace = toVkFrontFace(desc.renderState.frontFace);
        rsCi.depthBiasEnable = VK_FALSE;
        rsCi.depthBiasConstantFactor = 0;
        rsCi.depthBiasClamp = 0;
        rsCi.                                      depthBiasSlopeFactor = 0;
        rsCi.lineWidth = 1.0f;

        ci. pRasterizationState = &rsCi;
        //TODO: multi sample
        ci.pMultisampleState = 0;

        VkPipelineDepthStencilStateCreateInfo dsCi{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        dsCi.depthWriteEnable = desc.renderState.depthWriteEnable == true ? VK_TRUE : VK_FALSE;
        dsCi.                                  depthTestEnable = desc.renderState.depthTestEnable == true ?  VK_TRUE : VK_FALSE;
        dsCi.                                   depthCompareOp = toVkCompareOp(desc.renderState.depthCompareOp);
        dsCi.depthBoundsTestEnable = VK_FALSE;
        dsCi.                                  stencilTestEnable = desc.renderState.stencilTestEnable == true ? VK_TRUE : VK_FALSE;
        
        VkStencilOpState stState{};
        stState.    failOp =toVkStencilOp(desc.renderState.stencilFailOp);
        stState.    passOp = toVkStencilOp(desc.renderState.stencilPassOp);
        stState.depthFailOp = toVkStencilOp(desc.renderState.stencilDepthFailOp);
        stState.compareOp = toVkCompareOp(desc.renderState.stencilCompareOp);
        stState.compareMask = desc.renderState.stencilReadMask;
        stState.writeMask = desc.renderState.stencilWriteMask;
        stState.reference = desc.renderState.stencilReference;
        
        dsCi.front = stState;
        dsCi.back = stState;

        ci.pDepthStencilState = &dsCi;

        VkPipelineColorBlendAttachmentState defaultBlendState{
        .blendEnable = VK_FALSE
        };

        std::vector<VkPipelineColorBlendAttachmentState> attBlendStates(renderPass->passDesc.attachments.size(),defaultBlendState );
        for (int i = 0; i < desc.renderState.blendStates.size(); ++i) {
            auto& st = attBlendStates[i];
            auto& _st = desc.renderState.blendStates[i];
            st.blendEnable = _st.blendEnable == true ? VK_TRUE : VK_FALSE;
            st.srcColorBlendFactor = toVkBlendFactor(_st.srcColorBlend);
            st.dstColorBlendFactor = toVkBlendFactor(_st.dstColorBlend);
            st.colorBlendOp = toVkBlendOp(_st.colorBlendOp);
            st.srcAlphaBlendFactor = toVkBlendFactor(_st.srcAlphaBlend);
            st.dstAlphaBlendFactor = toVkBlendFactor(_st.dstAlphaBlend);
            st.alphaBlendOp = toVkBlendOp(_st.alphaBlendOp);
            st.colorWriteMask = VK_COLOR_COMPONENT_R_BIT| VK_COLOR_COMPONENT_G_BIT| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        }

        VkPipelineColorBlendStateCreateInfo bsCi{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        bsCi.                                      logicOpEnable = VK_FALSE;
        bsCi.logicOp = VK_LOGIC_OP_COPY;
        bsCi.attachmentCount = attBlendStates.size();
        bsCi.pAttachments = attBlendStates.data();
        float blendConsts[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        bsCi.blendConstants[0] = blendConsts[0];
        bsCi.blendConstants[1] = blendConsts[1];
        bsCi.blendConstants[2] = blendConsts[2];
        bsCi.blendConstants[3] = blendConsts[3];

        ci.pColorBlendState = &bsCi;

        VkPipelineDynamicStateCreateInfo dyStateCi{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dyStateCi.                             dynamicStateCount = ctx->pipelineDyStates.size();
        dyStateCi.pDynamicStates = ctx->pipelineDyStates.data();
        ci. pDynamicState = &dyStateCi;

        auto descritporSetInfos = getPipelineShaderInfo(desc.shaders);

        auto pipelineLayout = createPipelineLayoutVk(ctx, descritporSetInfos);

        if (!pipelineLayout)
        {
            assert(0 && "Null pipeline layout");
            return nullptr;
        }

        ci.layout = (VkPipelineLayout)pipelineLayout->native;
        ci.renderPass = (VkRenderPass)renderPass->native;

        //TODO:
        ci.subpass = 0;
        VkPipeline pipeline;

        VK_CHECK(vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1, &ci, 0,&pipeline), {
            return nullptr;
        });

        rs_pipeline_vk* ret = new rs_pipeline_vk;
        ret->native = pipeline;
        ret->renderState = desc.renderState;
        ret->type = PipelineType::Graphics;
        ret->vtxInput = desc.vertexInputDesc;
        ret->layout = pipelineLayout;
        return ret;
    }
    rs_pipeline_layout_vk* createPipelineLayoutVk(rs_context_vk* ctx, const std::vector<DescritporSetInfo>& descriptorInfos)
    {
        std::vector<std::pair<uint16_t, rs_descriptorset_layout_vk*>> setlayouts;
        for (auto&& setInfo : descriptorInfos) {
            std::pair<uint16_t, rs_descriptorset_layout_vk*> p;
            p.second = ctx->descriptorSetMgr->createDescriptorSetLayout(ctx, setInfo.layoutHash);
            p.first = setInfo.setIdx;
            setlayouts.push_back(p);
        }
        auto pipelineLayout = createPipelineLayout(ctx, setlayouts, {});
        return pipelineLayout;
    }

    void destroyPipelineLayout(rs_context_vk* ctx, rs_pipeline_layout_vk*& layout)
    {
        if (!layout) {
            assert(0 && "Null layout!");
            return;
        }
        for (auto&& i : layout->setLayouts) {
            ctx->descriptorSetMgr->returnDescriptorSetLayout(ctx,i.second);
        }
        delete layout;
        layout = 0;
    }
}