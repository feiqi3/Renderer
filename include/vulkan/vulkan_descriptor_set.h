#ifndef VULKAN_DESCRIPTOR_SET_H
#define VULKAN_DESCRIPTOR_SET_H

#include "vulkan_render_resource.h"
#include <set>
#include <vector>
#include <map>
#include <optional>
#include <cassert>
#include <algorithm>
#include <atomic>
#include <mutex>
namespace Render::Vulkan {

    constexpr int BITS_PER_BINDING = 11;
    constexpr int MAX_BINDINGS = 256 / BITS_PER_BINDING; // 21

    struct rs_vk_descriporset_layout_hash {

        struct AllocateHint {
            std::vector<uint16_t> hint;
        } mAllocaHint;

        std::vector< rs_descriptor> mDescriptors;
        uint64_t layoutHash = 0;
        inline bool checkValid() {
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

            mAllocaHint.hint.resize((int)ResourceType::Count);

            for (auto&& i : mDescriptors) {
                des = i;
                h ^= _;
                h *= FNV_prime;
                
                mAllocaHint.hint[(int)i.type]++;
            }

            layoutHash = h;
        }

    };

    struct rs_descriptorset_layout_vk : rs_base {


        inline void accquir() {
            ref++;
        }
        inline void release() {
            ref--;
            assert(ref >= 0 && "Wrong ref count");
        }

        std::atomic_uint32_t ref = 0;
        rs_vk_descriporset_layout_hash bindingHash;
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

    using PoolSizeInfo = std::vector<VkDescriptorPoolSize>;

    struct DescriptorPoolBlock {
        VkDescriptorPool         pool = VK_NULL_HANDLE;
        PoolSizeInfo sizes;
        uint32_t                  maxSets = 0;
        uint64_t lastActiveFrame = 0;
    };

    //TODO: per layout allocator
    class DescriptorSetManager {

    public:

        DescriptorSetManager(int maxFrame);
        void updateBufferData(uint64_t frame, rs_context_vk * ctx, rs_descriptorSet_vk * descriptorSet, int binding, void * data, int size, uint8_t queueType);
        void updateBufferBind(uint64_t frame, rs_context_vk* ctx, rs_descriptorSet_vk* descriptorSet, int binding, rs_buffer_vk* buffer);
        void updateBuffer(uint64_t frame, rs_context_vk* ctx, rs_descriptorSet_vk* descriptorSet, int binding, rs_buffer_vk* buffer, uint8_t queueType);
        void updateImage(uint64_t frame, rs_context_vk* ctx, rs_descriptorSet_vk* descriptorSet, int binding, rs_image_vk* image, uint8_t queueType);
        void updateSampler(uint64_t frame, rs_context_vk* ctx, rs_descriptorSet_vk* descriptorSet, int binding, rs_sampler_vk* sampler, uint8_t queueType);

        rs_pipeline_layout_vk* createFromShaders(rs_context_vk* ctx, std::vector<rs_shader_module_vk*>& shaders);

        rs_descriptorset_layout_vk* createDescriptorSetLayout(rs_context_vk* ctx,const rs_vk_descriporset_layout_hash& layoutHash);
        void returnDescriptorSetLayout(rs_context_vk* ctx, rs_descriptorset_layout_vk*& rs);
        VkDescriptorSetLayout getEmptyDescriptorSetLayout(rs_context_vk* ctx);

        rs_descriptorSet_vk AllocateDescriptorSet(uint64_t frame,rs_context_vk* ctx, rs_descriptorset_layout_vk* rs);
        
        void beginFrame(rs_context_vk* ctx, uint64_t frame);

        void endFrame(rs_context_vk* ctx, uint64_t frame);

    private:
        void destroyDescriptorSetLayout(rs_context_vk* ctx, rs_descriptorset_layout_vk* rs);
        DescriptorPoolBlock createNewPool(rs_context_vk* ctx);
        VkDescriptorSet tryAllocateFromPool(uint64_t frame,rs_context_vk* ctx,DescriptorPoolBlock& block,rs_descriptorset_layout_vk* layout);
    private:
        using LayoutMap = std::unordered_map<
            rs_vk_descriporset_layout_hash,  // key
            rs_descriptorset_layout_vk*,                    // value
            LayoutHash,                      // hash functor
            LayoutEqual                      // equal functor
        >;

        typedef std::list< DescriptorPoolBlock>  PoolList;

        std::mutex mMutex;
        LayoutMap mLayoutMap;
        std::vector<PoolList> m_pools;

        struct dyUBuffer {
            rs_buffer_vk* buffer;
            int freeSize;
            int maxSize;
            uint64_t lastActiveFrame = 0;
            uint8_t queueType = 0;
        };

        std::vector<
            std::list<dyUBuffer>
        > m_frameBuffers;
        
        dyUBuffer createNewDyBuffer(rs_context_vk* ctx, int size,uint8_t queueType);

        //TODO: create
        //Per pool
        PoolSizeInfo        m_defaultSize;
        std::vector<VkDescriptorPoolSize> m_defaultPoolAllocSize;
        uint32_t m_maxSet;
        
        VkDescriptorSetLayout mEmptyDescriptorSet = VK_NULL_HANDLE;
        int m_maxFrame = 1;

        inline static const int Max_Vacant_Frame = 10;
        inline static const int Max_Uniform_Buffer_Block_Size = 1024 * 256; //256k
    };

}

#endif