#ifndef VULKAN_DESCRIPTOR_SET_H
#define VULKAN_DESCRIPTOR_SET_H
#include <vector>
#include <unordered_map>
#include "vulkan_render_resource.h"
namespace Render::Vulkan {

    constexpr int BITS_PER_BINDING = 12;
    constexpr int MAX_BINDINGS = 256 / BITS_PER_BINDING; // 21

    struct DescriptorBinding {
        uint8_t bindingPos : 8;
        uint8_t descriptorType : 4;   // 0–15
        uint8_t descriptorCount : 4;  // 0–15
        uint8_t stageFlags : 4;
    };

    enum class DescriptorType : uint32_t {
        // 采样器
        Sampler = 0,
        // 绑定采样器与着色器可读图像
        CombinedImageSampler = 1,
        // 仅着色器可读图像
        SampledImage = 2,
        // 仅着色器可写图像
        StorageImage = 3,
        // 统一（只读）缓冲区（Vulkan 中的 UBO）
        UniformBuffer = 4,
        // 存储（可读写）缓冲区（Vulkan 中的 SSBO）
        StorageBuffer = 5,
        // 统一缓冲区动态偏移（Vulkan 的 DYNAMIC UBO）
        UniformBufferDynamic = 6,
        // 存储缓冲区动态偏移（Vulkan 的 DYNAMIC SSBO）
        StorageBufferDynamic = 7,
        // 统一（只读）纹理缓冲（Texel Buffer）
        UniformTexelBuffer = 8,
        // 存储（可读写）纹理缓冲（Texel Buffer）
        StorageTexelBuffer = 9,
        // 输入附件（Vulkan 中的 Input Attachment）
        InputAttachment = 10,
        // 额外：加速结构（用于 Ray Tracing）
        AccelerationStructure = 11,
        // 保留用于未来扩展
        Count = 12
    };

    VkDescriptorType toVkDescriptorType(DescriptorType type);

    std::vector<DescriptorBinding> fromHashToBinding(const rs_vk_descriporset_layout_hash& hash);

    rs_vk_descriporset_layout_hash fromBindingToHash(const std::vector<DescriptorBinding>& bindings);

    struct LayoutHashHasher {
        std::size_t operator()(rs_vk_descriporset_layout_hash const& h) const noexcept {
            // 64位平台上 size_t=64位，可直接混合
            uint64_t seed = 0xcbf29ce484222325ULL; // FNV offset basis
            for (auto v : h.data) {
                // FNV-1a 64-bit
                seed ^= v;
                seed *= 0x100000001b3ULL;
            }
            return static_cast<std::size_t>(seed);
        }
    };

    struct LayoutHashEqual {
        bool operator()(rs_vk_descriporset_layout_hash const& a,
            rs_vk_descriporset_layout_hash const& b) const noexcept {
            return a.data == b.data;
        }
    };

    class DescriptorSetManager {

    public:
        using DescirptorSetLayoutMap = std::unordered_map< rs_vk_descriporset_layout_hash, rs_descriptorset_layout_vk, LayoutHashHasher, LayoutHashEqual>;
        

        VkDescriptorSetLayout getEmptySetlayoout();
        DescirptorSetLayoutMap m_descriptorsetLayoutMap;

    public:
        VkDescriptorSetLayout m_emptyLayout;
    };

}

#endif