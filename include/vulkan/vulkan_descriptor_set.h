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


    struct UniformBufferObject :rs_base {

        struct FrameAllocateInfo {
            uint32_t beginPos = 0;
            uint32_t endPos = 0;
            uint32_t allocateSize = 0;
        };
        QueueType queue = QueueType_Graphics;
        uint32_t mMaxSize = 0;
        uint32_t mHead = 0;
        uint32_t mTail = 0;
        uint32_t mAlignedSize = 32;
        std::atomic_uint32_t mUsingNum = 0;
        uint64_t mLastActive = 0;
        std::vector< FrameAllocateInfo> mAllocatePerFrame;

        UniformBufferObject(uint32_t maxFrameInFlight) {
            mAllocatePerFrame.resize(maxFrameInFlight);
        }

        inline uint32_t usedBytes() const {
            if (mMaxSize == 0) return 0;
            if (mHead >= mTail) return mHead - mTail;
            return mMaxSize - (mTail - mHead);
        }

        inline uint32_t freeBytes() const {
            if (mMaxSize == 0) return 0;
            return mMaxSize - usedBytes();
        }

        static inline uint32_t alignUp(uint32_t v, uint32_t align) {
            if (align == 0) return v;
            uint32_t rem = v % align;
            return rem == 0 ? v : v + (align - rem);
        }

        inline bool pushBytes(void* data, uint32_t bytes, uint32_t frameInFlight, uint64_t frame,uint32_t& outOffset) {
            auto& frameAllocateInfo = mAllocatePerFrame[frameInFlight];
            if (frameAllocateInfo.allocateSize == 0) {
                frameAllocateInfo.beginPos = mHead;
            }

            uint32_t beginOffsetPos = 0;
            bool canAllocate = pushBytes(bytes, beginOffsetPos);
            if (!canAllocate)return false;
            auto buffer = (rs_buffer_vk*)native;
            memcpy((uint8_t*)(buffer->mappedPtr) + beginOffsetPos, data, bytes);
            frameAllocateInfo.endPos = mHead;
            frameAllocateInfo.allocateSize += bytes;
            mLastActive = frame;
            outOffset = beginOffsetPos;
            return true;
        }

        inline bool releaseFrameBytes(uint32_t frameInFlight) {
            FrameAllocateInfo& frameAllocateInfo = mAllocatePerFrame[frameInFlight];
            if (frameAllocateInfo.beginPos != mTail) {
                assert(0);
                return false;
            }
            mTail = frameAllocateInfo.endPos;
            frameAllocateInfo = {};
            return true;
        }
    private:

        inline bool pushBytes(uint32_t bytes, uint32_t& outOffset) {
            if (bytes == 0) return false;
            if (mMaxSize == 0) return false;
            if (bytes > mMaxSize) return false; // 请求超过总容量，必失败

            // 计算两种情形：head >= tail（空或尾在前），head < tail（中间有可连续空间）
            if (mHead >= mTail) {
                // 末尾到 buffer 末尾的连续空间
                uint32_t spaceAtEnd = mMaxSize - mHead;

                // 先尝试在末尾分配（需要对齐 padding）
                uint32_t alignedHead = alignUp(mHead, mAlignedSize);
                uint32_t pad = (alignedHead >= mHead) ? (alignedHead - mHead) : 0;

                // 若对齐后位置超出 buffer 末尾，则等同于末尾空间不足，尝试环回到 0
                if (pad > spaceAtEnd) {
                    // 环回到前端(从 0 开始分配)
                }
                else {
                    // 检查末尾是否能容纳（pad + bytes）
                    if (pad + bytes <= spaceAtEnd) {
                        outOffset = alignedHead;
                        // new head = outOffset + bytes (mod mMaxSize)
                        uint32_t newHead = alignedHead + bytes;
                        if (newHead == mMaxSize) newHead = 0; // exactly at end -> wrap to 0
                        mHead = newHead;
                        return true;
                    }
                    // 否则尝试环回到 0
                }

                // 尝试从 0 开始（因为尾部不足），分配在开头
                // 在 0 处对齐总是 0（因为 0 % align == 0）
                // 可用前端空间 = mTail (tail 开始处被占用，因此可用到 tail-1)
                uint32_t spaceAtFront = mTail; // note: if tail == 0, spaceAtFront == 0
                if (bytes <= spaceAtFront) {
                    outOffset = 0;
                    mHead = bytes; // 因为从 0 开始分配，head 挪到 bytes 处
                    return true;
                }

                // 仍然不够，失败
                return false;
            }
            else { // mHead < mTail, 可用的连续空间在 head..tail-1
                uint32_t contiguousFree = mTail - mHead;
                uint32_t alignedHead = alignUp(mHead, mAlignedSize);
                uint32_t pad = alignedHead - mHead; // alignedHead > mHead, pad < contiguousFree maybe

                if (pad + bytes <= contiguousFree) {
                    outOffset = alignedHead;
                    mHead = alignedHead + bytes; // 这里不会超出 mMaxSize，因为 alignedHead + bytes < mTail <= mMaxSize-1
                    return true;
                }
                else {
                    return false;
                }
            }
        }

    };

    struct rs_vk_descriporset_layout_hash {

        struct AllocateHint {
            std::vector<uint16_t> hint;
        } mAllocaHint;

        std::vector< rs_descriptor> mDescriptors;
        uint64_t layoutHash = 0;
        uint8_t maxBinding = 0;
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
            for (auto&& set : mDescriptors) {
                maxBinding = std::max(maxBinding, set.binding);
            }
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
        uint32_t mInUseNum = 0;
        uint32_t mReturnedNum = 0;
    };

    //TODO: per layout allocator
    class DescriptorSetManager {

    public:

        DescriptorSetManager(rs_context_vk* ctx,int maxFrame);
        void updateBufferData(uint64_t frame, rs_context_vk * ctx, rs_descriptorSet_vk * descriptorSet, int binding, void * data, int size, QueueType queueType);
        void updateBufferBind(uint64_t frame, rs_context_vk* ctx, rs_descriptorSet_vk* descriptorSet, int binding, rs_buffer_vk* buffer);
        void updateBuffer(uint64_t frame, rs_context_vk* ctx, rs_descriptorSet_vk* descriptorSet, int binding, rs_buffer_vk* buffer, uint8_t queueType);
        void updateImage(uint64_t frame, rs_context_vk* ctx, rs_descriptorSet_vk* descriptorSet, int binding, rs_image_vk* image, uint8_t queueType);
        void updateSampler(uint64_t frame, rs_context_vk* ctx, rs_descriptorSet_vk* descriptorSet, int binding, rs_sampler_vk* sampler, uint8_t queueType);

        rs_pipeline_layout_vk* createFromShaders(rs_context_vk* ctx, std::vector<rs_shader_module_vk*>& shaders);

        rs_descriptorset_layout_vk* createDescriptorSetLayout(rs_context_vk* ctx,const rs_vk_descriporset_layout_hash& layoutHash);
        void returnDescriptorSetLayout(rs_context_vk* ctx, rs_descriptorset_layout_vk*& rs);
        VkDescriptorSetLayout getEmptyDescriptorSetLayout(rs_context_vk* ctx);

        rs_descriptorSet_vk* AllocateDescriptorSet(uint64_t frame, rs_context_vk* ctx, rs_pipeline_vk* pipeline,uint32_t setIdx);
        rs_descriptorSet_vk* AllocateDescriptorSet(uint64_t frame,rs_context_vk* ctx, rs_descriptorset_layout_vk* rs);
        void ReturnDescriptorSet(uint64_t frame, rs_context_vk* ctx, rs_descriptorSet_vk*& descriptorSet);

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
        std::vector<VkDescriptorPoolSize> m_defaultPoolAllocSize;
        uint32_t m_maxSet = 50;
        
        VkDescriptorSetLayout mEmptyDescriptorSet = VK_NULL_HANDLE;
        int m_maxFrame = 1;

        inline static const int Max_Vacant_Frame = 10;
        inline static const int Max_Uniform_Buffer_Block_Size = 1024 * 256; //256k
    public:

    private:
        std::list< UniformBufferObject* > mUniformBufferLists;
        std::mutex mAllocateUniformBufferLock;
        UniformBufferObject* createUBO(rs_context_vk* ctx, uint64_t createSize,uint32_t alignedSize,QueueType queue);
        void updateDynamicBuffer(uint64_t frame, rs_context_vk* ctx, rs_descriptorSet_vk* descriptorSet,int binding, void* data, uint64_t size, QueueType queue);
    };

}

#endif