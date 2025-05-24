#include "vulkan/vulkan_descriptor_set.h"
#include "vulkan/vulkan_render_function.h"
#include <map>
namespace Render::Vulkan {
    namespace {

    }


    std::optional<std::vector<rs_descriptor>> toDescriptors(const std::vector<std::vector<rs_descriptor>>& desc)
    {
        std::map<uint16_t,rs_descriptor> bindings;
        for (auto&& des : desc) {
            for (auto&& descriptor : des) {
                auto itor = bindings.find(descriptor.binding);
                if (itor == bindings.end()) {
                    bindings.insert({ descriptor.binding,descriptor });
                }
                else {
                    auto& oldDesc = itor->second;
                    if (oldDesc.count == descriptor.count
                        && oldDesc.type == descriptor.type
                        ) {
                        oldDesc.shaderVisibleStage |= descriptor.shaderVisibleStage;
                    }
                    else {
                        return std::nullopt;
                    }
                }
            }
        }
        std::vector<rs_descriptor> ret;
        ret.reserve(bindings.size());
        for (auto&& [_, descriptor] : bindings) {
            ret.push_back(descriptor);
        }
        return ret;
    }

    VkDescriptorType toVkDescriptorType(ResourceType type) {
        switch (type) {
        case ResourceType::UniformBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        case ResourceType::StorageBuffer:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case ResourceType::StorageImage:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case ResourceType::Texture:
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case ResourceType::InputAttachment:
            return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        case ResourceType::Sampler:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
        case ResourceType::AccelerationStructure:
            return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        default:
            return VK_DESCRIPTOR_TYPE_MAX_ENUM; // or handle error
        }
    }

    void DescriptorSetManager::returnDescriptorSetLayout(rs_context_vk* ctx, rs_descriptorset_layout_vk*& rs)
    {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            auto itor = mLayoutMap.find(rs->bindingHash);
            if (itor != mLayoutMap.end()) {
                itor->second->release();
                if (itor->second->ref == 0) {
                    mLayoutMap.erase(itor);
                }
            }
            else {
                assert(0 && "Not Managered set layout");
            }

            destroyDescriptorSetLayout(ctx,rs);

        }
    }

    VkDescriptorSetLayout DescriptorSetManager::getEmptyDescriptorSetLayout(rs_context_vk* ctx)
    {
        if (mEmptyDescriptorSet == VK_NULL_HANDLE) {
            VkDescriptorSetLayoutCreateInfo ci{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
            VK_CHECK(vkCreateDescriptorSetLayout(ctx->device, &ci, 0, &mEmptyDescriptorSet), {
                    uint64_t i = 0;
                    //Die
                    *(int*)(&i);
            });
        }
        return mEmptyDescriptorSet;
    }

    rs_descriptorset_layout_vk* DescriptorSetManager::createDescriptorSetLayout(rs_context_vk* ctx, const rs_vk_descriporset_layout_hash& layoutHash)
    {
        VkDescriptorSetLayoutCreateInfo ci{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
        };
        {
            std::lock_guard<std::mutex> lock(mMutex);
            auto itor = mLayoutMap.find(layoutHash);
            if (itor != mLayoutMap.end()) {
                itor->second->accquir();
                return itor->second;
            }
        }
        auto& bindings = layoutHash.mDescriptors;
        ci.bindingCount = bindings.size();
        std::vector< VkDescriptorSetLayoutBinding> vkBindings;
        vkBindings.reserve(bindings.size());

        for (auto&& binding : bindings) {
            VkDescriptorSetLayoutBinding b{};
            b.binding = binding.binding;
            b.descriptorCount = binding.count;
            b.descriptorType = toVkDescriptorType(binding.type);
            b.stageFlags = toVkShaderStageFlags(binding.shaderVisibleStage);
            b.pImmutableSamplers = 0;
            vkBindings.push_back(b);
        }
        ci.pBindings = vkBindings.data();
        VkDescriptorSetLayout layout;
        VK_CHECK(vkCreateDescriptorSetLayout(ctx->device, &ci, 0, &layout), {
            return nullptr;
        });
        rs_descriptorset_layout_vk* l = new rs_descriptorset_layout_vk;
        l->native = layout;

        {
            std::lock_guard<std::mutex> lock(mMutex);
            auto itor = mLayoutMap.find(layoutHash);
            if (itor != mLayoutMap.end()) {
                destroyDescriptorSetLayout(ctx, l);
                itor->second->accquir();
                return itor->second;
            }
            l->bindingHash = layoutHash;
            this->mLayoutMap.insert({
                layoutHash,l
            });
            l->accquir();
        }

        return l;
    }

    void DescriptorSetManager::destroyDescriptorSetLayout(rs_context_vk* ctx, rs_descriptorset_layout_vk* rs)
    {
        VkDescriptorSetLayout setLayout = (VkDescriptorSetLayout)rs->native;
        vkDestroyDescriptorSetLayout(ctx->device, setLayout, 0);
        delete rs;
        rs = 0;
    }
}