#ifndef VULKAN_DESCRIPTOR_SET_H
#define VULKAN_DESCRIPTOR_SET_H
#include <vector>
#include <unordered_map>
#include <optional>
#include "vulkan_render_resource.h"
#include <set>
namespace Render::Vulkan {

    constexpr int BITS_PER_BINDING = 11;
    constexpr int MAX_BINDINGS = 256 / BITS_PER_BINDING; // 21

    struct rs_vk_descriporset_layout_hash {
        std::vector< rs_descriptor> mDescriptors;
        uint64_t layoutHash = 0;
        bool checkValid() {
            std::set<uint8_t> bindings;
            for (auto&& i : mDescriptors) {
                if (bindings.find(i.binding) != bindings.end()) {
                    return false;
                }
                bindings.insert(i.binding);
            }
            return true;
        }
        
        inline void init() {
            assert(checkValid());
            std::map<int, rs_descriptor> mapSet;

            std::sort(mDescriptors.begin(), mDescriptors.end(), [](const auto& a, const auto& b) {
                return a.binding > b.binding;
             });
            union
            {
                uint64_t _ = 0;
                rs_descriptor des;
            };
            const uint64_t FNV_offset = 0xcbf29ce484222325ULL;
            const uint64_t FNV_prime = 0x100000001b3ULL;

            uint64_t h = FNV_offset;

            for (auto&& i : mDescriptors) {
                des = i;
                h ^= _;
                h *= FNV_prime;
            }
            layoutHash = h;
        }
    };

    VkDescriptorType toVkDescriptorType(ResourceType resType);

    std::optional<std::vector<rs_descriptor>> toDescriptors(const std::vector<std::vector<rs_descriptor>>& desc);

    struct LayoutHash {
        size_t operator()(rs_vk_descriporset_layout_hash const& L) const noexcept {
            return static_cast<size_t>(L.layoutHash);
        }
    };

    struct LayoutEqual {
        bool operator()(rs_vk_descriporset_layout_hash const& A,
            rs_vk_descriporset_layout_hash const& B) const noexcept
        {
            if (A.layoutHash != B.layoutHash)
                return false;
            if (A.mDescriptors.size() != B.mDescriptors.size())
                return false;
            for (size_t i = 0; i < A.mDescriptors.size(); ++i) {
                auto const& a = A.mDescriptors[i];
                auto const& b = B.mDescriptors[i];
                if (a.binding != b.binding ||
                    a.type != b.type ||
                    a.shaderVisibleStage != b.shaderVisibleStage ||
                    a.count != b.count)
                {
                    return false;
                }
            }
            return true;
        }
    };

    class DescriptorSetManager {

    public:
        rs_descriptorset_layout_vk* createDescriptorSetLayout(rs_context_vk* ctx,const rs_vk_descriporset_layout_hash& layoutHash);
        void returnDescriptorSetLayout(rs_context_vk* ctx, rs_descriptorset_layout_vk*& rs);
        VkDescriptorSetLayout getEmptyDescriptorSetLayout(rs_context_vk* ctx);
    private:
        using LayoutMap = std::unordered_map<
            rs_vk_descriporset_layout_hash,  // key
            rs_descriptorset_layout_vk*,                    // value
            LayoutHash,                      // hash functor
            LayoutEqual                      // equal functor
        >;
        std::mutex mMutex;
        LayoutMap mLayoutMap;
        void destroyDescriptorSetLayout(rs_context_vk* ctx, rs_descriptorset_layout_vk*& rs);

        VkDescriptorSetLayout mEmptyDescriptorSet = VK_NULL_HANDLE;
    };

}

#endif