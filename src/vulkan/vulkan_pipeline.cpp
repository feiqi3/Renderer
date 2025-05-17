#include "vulkan/vulkan_pipeline.h"
#include "vulkan/vulkan_descriptor_set.h"
#include "vulkan/vulkan_render_resource.h"
#include "vulkan_render_function.cpp"
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


    rs_pipeline_layout_vk* createPipelineLayout(rs_context_vk* context, const std::vector<std::pair<uint16_t, rs_descriptorset_layout_vk*>>& setLayouts, const std::vector<VkPushConstantRange>& pushConstants)
    {
        
        uint32_t maxSets = 0;
        
        for (auto&& [binding, setLayout] : setLayouts) {
            setLayout->accquir();
            maxSets = std::max(uint32_t(binding), maxSets);
        }
        std::vector<VkDescriptorSetLayout> setlayout_vks;
        std::fill(setlayout_vks.begin(), setlayout_vks.end(), context->descriptorSetMgr->getEmptySetlayoout());

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
}