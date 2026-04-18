#ifndef VULKAN_DESCRIPTOR_SET_H
#define VULKAN_DESCRIPTOR_SET_H

#include <set>
#include <vector>
#include <map>
#include <optional>
#include <cassert>
#include <algorithm>
#include <atomic>
#include <mutex>
#include "vulkan_render_resource.h"
#include "common/RingBuffer.h"
#include "render_log.h"
namespace Render::Vulkan {

    constexpr int BITS_PER_BINDING = 11;
    constexpr int MAX_BINDINGS = 256 / BITS_PER_BINDING; // 21

    struct UniformBufferObject :rs_base {
    public:
        UniformBufferObject( uint32_t maxFrameInFlight, uint32_t bufferSize, uint32_t alignedSize);
        void releaseFrame(uint32_t frameInFlight);
        bool allocateInFrame(void* data, uint32_t size, uint32_t frameInFlight,uint64_t frame,uint32_t& offsetInBuffer);

    public:
        rs_buffer_vk* mBuffer = 0;
        uint64_t mLastActiveFrame = 0;
        std::atomic_int32_t mInUsedNum = 0;
        QueueType mQueue = QueueType::QueueType_Graphics;

    private:
        Render::Common::RingBufferAllocator mRingBufferAllocator;
        struct FrameAllocateInfo {
            uint32_t headOffset = 0;
            uint32_t totalAdvanceSize = 0;
        };
        std::vector<FrameAllocateInfo> mFrameAllocateInfo;
        uint32_t mAlignment = 0;
    };

    struct rs_vk_descriporset_layout_hash {

        struct AllocateHint {
            std::vector<uint16_t> hint; //How many descriptors should be allocated?   
        } mAllocaHint;

        std::vector< rs_descriptor> mDescriptors;
        uint64_t layoutHash = 0;
        uint16_t maxBinding = 0;
        inline bool checkValid() {
            std::set<uint16_t> bindings;
            for (auto&& i : mDescriptors) {
                auto vk_binding_pos = toVkBindingPos(i.bindingPos);
                if (bindings.find((vk_binding_pos.bindingIdx)) != bindings.end()) {
                    return false;
                }
                bindings.insert(vk_binding_pos.bindingIdx);
            }
            return true;
        }
        
        inline const rs_descriptor& getDescriptor(int bindingIndex) const {
            if (mDescriptors.size() <= bindingIndex) {
                static rs_descriptor errorDescriptor{.type = UniformType::Count};
                return errorDescriptor;
            }
            return mDescriptors[bindingIndex];
        }

        inline void init() {
            assert(checkValid());
            std::map<int, rs_descriptor> mapSet;
            for (auto&& set : mDescriptors) {
                auto vk_binding_pos = toVkBindingPos(set.bindingPos);
                vk_binding_pos.setIdx = -1;
                set.bindingPos = toRsBindingPos(vk_binding_pos);
                maxBinding = std::max(maxBinding, uint16_t(vk_binding_pos.bindingIdx));
            }
            rs_descriptor defaultDescriptor{};
            defaultDescriptor.type = UniformType::Count;
            std::vector< rs_descriptor> copyDescriptors(maxBinding + 1,defaultDescriptor);

            for (const rs_descriptor& descriptor : mDescriptors) {
                vk_binding_pos bindingPos = toVkBindingPos(descriptor.bindingPos);
                if (copyDescriptors[bindingPos.bindingIdx].type != UniformType::Count) {
                    Log::error("Data corrupt data in 'rs_vk_descriporset_layout_hash', Check at binding: { " + std::to_string(bindingPos.bindingIdx) + " }");
                    assert(false);
                }
                copyDescriptors[bindingPos.bindingIdx] = descriptor;
            }
            mDescriptors = std::move(copyDescriptors);
			//Now we can fetch descriptor directly by treating binding index as subscript of descriptor array



            struct {
                uint32_t binding = 0;
                uint16_t shaderVisibleStage = 0; //shader stage
                uint16_t count = 0;
                uint16_t size = 0;
                UniformType type = UniformType::Count;
            }descripor;

            const uint64_t FNV_offset = 0xcbf29ce484222325ULL;
            const uint64_t FNV_prime = 0x100000001b3ULL;

            uint64_t h = FNV_offset;

            mAllocaHint.hint.resize((int)UniformType::Count);

            for (auto&& i : mDescriptors) {
                if (i.type == UniformType::Count)continue;
                descripor.binding = i.bindingPos;
                descripor.shaderVisibleStage = i.shaderVisibleStage;
                descripor.count = i.count;
                descripor.size = i.size;
                descripor.type = i.type;
                uint8_t* data = (uint8_t*) & descripor;
                for (auto byteNum = 0; byteNum < sizeof(descripor); ++byteNum) {
                    h ^= data[byteNum];
                    h *= FNV_prime;
                }
                
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

    VkDescriptorType toVkDescriptorType(UniformType resType);

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
                auto bindingInfoA = toVkBindingPos(a.bindingPos);
                auto bindingInfoB = toVkBindingPos(b.bindingPos);

                if (bindingInfoA.bindingIdx != bindingInfoB.bindingIdx ||
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
        uint32_t mInUseNum = 0;
        uint32_t mReturnedNum = 0;
    };

    //TODO: per layout allocator
    class DescriptorSetManager {

    public:
        DescriptorSetManager(rs_context_vk* ctx,int maxFrame);

        rs_pipeline_layout_vk* createFromShaders(rs_context_vk* ctx, std::vector<rs_shader_module_vk*>& shaders);

        rs_descriptorset_layout_vk* createDescriptorSetLayout(rs_context_vk* ctx,const rs_vk_descriporset_layout_hash& layoutHash);
        void returnDescriptorSetLayout(rs_context_vk* ctx, rs_descriptorset_layout_vk*& rs);
        VkDescriptorSetLayout getEmptyDescriptorSetLayout(rs_context_vk* ctx);

        rs_descriptorSet_vk* AllocateDescriptorSet(uint64_t frame, rs_context_vk* ctx, rs_graphic_pipeline_vk* pipeline,uint32_t setIdx);
        rs_descriptorSet_vk* AllocateDescriptorSet(uint64_t frame,rs_context_vk* ctx, rs_descriptorset_layout_vk* rs);
        void ReturnDescriptorSet(rs_context_vk* ctx, rs_descriptorSet_vk*& descriptorSet);

        void beginFrame(rs_context_vk* ctx, uint64_t frame);

        void endFrame(rs_context_vk* ctx, uint64_t frame);

    private:
        void destroyDescriptorSetLayout(rs_context_vk* ctx, rs_descriptorset_layout_vk* rs);
        DescriptorPoolBlock* createNewPool(rs_context_vk* ctx);
        VkDescriptorSet tryAllocateFromPool(uint64_t frame,rs_context_vk* ctx,DescriptorPoolBlock* block,rs_descriptorset_layout_vk* layout);

    private:
        using LayoutMap = std::unordered_map<
            rs_vk_descriporset_layout_hash,  // key
            rs_descriptorset_layout_vk*,                    // value
            LayoutHash,                      // hash functor
            LayoutEqual                      // equal functor
        >;

        typedef std::list< DescriptorPoolBlock*>  PoolList;

        std::mutex mMutex;
        LayoutMap mLayoutMap;
        std::vector<PoolList> m_pools;
        
        //TODO: create
        //Per pool
        PoolSizeInfo        m_defaultSize;
        uint32_t m_maxSet = 50;
        
        VkDescriptorSetLayout mEmptyDescriptorSet = VK_NULL_HANDLE;
        int m_maxFrame = 1;
        int curFif = -1;
        inline static const int Max_Vacant_Frame = 10;
        inline static const int Max_Uniform_Buffer_Block_Size = 1024 * 256; //256k
    public:
        std::pair<UniformBufferObject*, uint32_t>getCurFrameDefaultUBO();
        std::pair<UniformBufferObject*, uint32_t> getDybuffer(uint64_t frame, uint32_t fif, rs_context_vk* ctx, void* data, uint64_t size, QueueType queue, UniformBufferObject* formerUBO);
        void returnDybuffer(UniformBufferObject* ubo);
    private:
        UniformBufferObject* curFrameDefaultUBO = nullptr;
        uint32_t curFrameDyOffset = 0;
        std::list< UniformBufferObject* > mUniformBufferLists;
        std::mutex mAllocateUniformBufferLock;
        UniformBufferObject* createUBO(rs_context_vk* ctx, uint64_t createSize,uint32_t alignedSize,QueueType queue);
        void destroyUBO(rs_context_vk* ctx, UniformBufferObject* ubo);
        void updateDynamicBuffer(uint64_t frame, rs_context_vk* ctx, rs_descriptorSet_vk* descriptorSet,int binding, void* data, uint64_t size, QueueType queue);
    };

}

#endif