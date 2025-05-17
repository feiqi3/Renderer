#include "vulkan/vulkan_descriptor_set.h"

namespace Render::Vulkan {
    namespace {
        uint32_t extractBits(const rs_vk_descriporset_layout_hash& hash,
            int bitOffset,
            int bitWidth) {
            assert(bitWidth <= 32);
            int idx = bitOffset / 64;
            int offset64 = bitOffset % 64;
            uint64_t low = hash.data[idx] >> offset64;
            int bitsLow = std::min(64 - offset64, bitWidth);
            uint64_t value = low & ((uint64_t(1) << bitsLow) - 1);

            if (bitsLow < bitWidth) {
                // 跨越到下一个 uint64
                uint64_t high = hash.data[idx + 1] & ((uint64_t(1) << (bitWidth - bitsLow)) - 1);
                value |= high << bitsLow;
            }
            return static_cast<uint32_t>(value);
        }

        void insertBits(rs_vk_descriporset_layout_hash& key,
            uint32_t value,
            int bitOffset,
            int bitWidth)
        {
            assert(bitWidth <= 32);
            int idx = bitOffset / 64;
            int offset64 = bitOffset % 64;
            uint64_t mask = (bitWidth == 64 ? ~0ull : ((1ull << bitWidth) - 1));
            uint64_t v = uint64_t(value) & mask;

            // 跨越两个 uint64
            if (offset64 + bitWidth > 64) {
                int lowBits = 64 - offset64;
                int highBits = bitWidth - lowBits;
                key.data[idx] |= (v & ((1ull << lowBits) - 1)) << offset64;
                key.data[idx + 1] |= v >> lowBits;
            }
            else {
                key.data[idx] |= v << offset64;
            }
        }

        void fromBindingToHash(rs_vk_descriporset_layout_hash& hash,
            const DescriptorBinding& b
        )
        {
            assert(b.bindingPos < MAX_BINDINGS);

            // 计算该 binding 在哈希中的起始位偏移
            int bitOffset = int(b.bindingPos) * BITS_PER_BINDING;

            // 拼成 12 位整数：[type:4][count:4][stage:4]
            uint32_t v = (uint32_t(b.descriptorType) << 8)
                | (uint32_t(b.descriptorCount) << 4)
                | (uint32_t(b.stageFlags) << 0);

            insertBits(hash, v, bitOffset, BITS_PER_BINDING);
        }
    }

	VkDescriptorType toVkDescriptorType(DescriptorType type)
    {
        switch (type) {
        case DescriptorType::Sampler:                 return VK_DESCRIPTOR_TYPE_SAMPLER;
        case DescriptorType::CombinedImageSampler:    return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case DescriptorType::SampledImage:            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case DescriptorType::StorageImage:            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case DescriptorType::UniformBuffer:           return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case DescriptorType::UniformBufferDynamic:    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        case DescriptorType::StorageBuffer:           return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case DescriptorType::StorageBufferDynamic:    return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        case DescriptorType::UniformTexelBuffer:      return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        case DescriptorType::StorageTexelBuffer:      return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        case DescriptorType::InputAttachment:         return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        case DescriptorType::AccelerationStructure:   return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        default:                                      return VK_DESCRIPTOR_TYPE_MAX_ENUM;
        }
    }

    std::vector<DescriptorBinding> fromHashToBinding(const rs_vk_descriporset_layout_hash& hash)
    {
        std::vector<DescriptorBinding> result;
        result.reserve(MAX_BINDINGS);

        int bitOffset = 0;
        for (int i = 0; i < MAX_BINDINGS; ++i) {
            uint32_t v = extractBits(hash, bitOffset, BITS_PER_BINDING);
            bitOffset += BITS_PER_BINDING;

            uint8_t type = (v >> 8) & 0xF;
            uint8_t count = (v >> 4) & 0xF;
            uint8_t stage = (v >> 0) & 0xF;

            if (count == 0)
                continue;

            DescriptorBinding b;
            b.bindingPos = i;
            b.descriptorType = type;
            b.descriptorCount = count;
            b.stageFlags = stage;
            result.push_back(b);
        }
        return result;
    }



    rs_vk_descriporset_layout_hash fromBindingToHash(const std::vector<DescriptorBinding>& bindings)
    {
        rs_vk_descriporset_layout_hash hash;
        for (auto&& i : bindings) {
            fromBindingToHash(hash,i);
        }
        return hash;
    }
    VkDescriptorSetLayout DescriptorSetManager::getEmptySetlayoout()
    {
        return m_emptyLayout;
    }
}