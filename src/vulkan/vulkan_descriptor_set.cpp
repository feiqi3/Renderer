#include "vulkan/vulkan_descriptor_set.h"
#include "vulkan/vulkan_render_function.h"
#include "render_log.h"
#include "vulkan/vulkan_resource_state.h"
#include <vulkan/vulkan_pipeline.h>
#include <map>

#define DESCRIPTOR_VAR_COUNT_SIZE 1024 * 4
namespace Render::Vulkan {
    namespace {
        const uint64_t RingBufferSize = 1024 * 32; //32KB
        uint64_t RingBufferAlignedSize = 0; //16B
        using UBO = UniformBufferObject;
        /*
        VUID-vkCmdBindDescriptorSets-pDynamicOffsets-01971
        Each element of pDynamicOffsets which corresponds to a descriptor binding with type VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC must be a multiple of VkPhysicalDeviceLimits::minUniformBufferOffsetAlignment
        */
    }

    VkDescriptorType toVkDescriptorType(UniformType type) {
        switch (type) {
        case UniformType::ConstantBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case UniformType::UniformBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        case UniformType::StorageBuffer:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case UniformType::StorageImage:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case UniformType::Texture:
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case UniformType::InputAttachment:
            return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        case UniformType::Sampler:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
        //case UniformType::AccelerationStructure:
        //    return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        default:
            return VK_DESCRIPTOR_TYPE_MAX_ENUM; // or handle error
        }
    }

    DescriptorSetManager::DescriptorSetManager(rs_context_vk* ctx, int maxFrame)
    {
        RingBufferAlignedSize = ctx->physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
        m_maxFrame = maxFrame;
        this->m_pools.resize(maxFrame);
        VkDescriptorPoolSize poolFactor{};

        poolFactor.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolFactor.descriptorCount = 24;
        this->m_defaultSize.push_back(poolFactor);

        poolFactor.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        poolFactor.descriptorCount = 60;
        this->m_defaultSize.push_back(poolFactor);

        poolFactor.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolFactor.descriptorCount = 60;
        this->m_defaultSize.push_back(poolFactor);

        poolFactor.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        poolFactor.descriptorCount = 60;
        this->m_defaultSize.push_back(poolFactor);

        poolFactor.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        poolFactor.descriptorCount = 60;
        this->m_defaultSize.push_back(poolFactor);

        poolFactor.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        poolFactor.descriptorCount = 60;
        this->m_defaultSize.push_back(poolFactor);

        poolFactor.type = VK_DESCRIPTOR_TYPE_SAMPLER;
        poolFactor.descriptorCount = 60;
        this->m_defaultSize.push_back(poolFactor);

        //poolFactor.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        //poolFactor.descriptorCount = 10;
        //this->m_defaultSize.push_back(poolFactor);

    }

    VkDescriptorSet DescriptorSetManager::tryAllocateFromPool(uint64_t frame,rs_context_vk* ctx, DescriptorPoolBlock* block, rs_descriptorset_layout_vk* layout) {
        auto& type = layout->bindingHash.mDescriptors;

        if (block->maxSets == 0)
            return VK_NULL_HANDLE;

        for (auto i = 0; i < (int)UniformType::Count; ++i) {

            if (block->sizes[i].descriptorCount < layout->bindingHash.mAllocaHint.hint[i]) {
                return VK_NULL_HANDLE;
            }
        }

        for (auto i = 0; i < (int)UniformType::Count; ++i) {
            block->sizes[i].descriptorCount -= layout->bindingHash.mAllocaHint.hint[i];
        }
        block->maxSets--;

        VkDescriptorSetAllocateInfo allci{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allci.                descriptorPool = block->pool;
        allci.                        descriptorSetCount = 1;
        VkDescriptorSetLayout _layout = (VkDescriptorSetLayout)layout->native;
        allci.pSetLayouts = &_layout;
        VkDescriptorSet desSet;

        if (layout->bindingHash.DescriptorSetFlags & DescriptorSetVarCountBindingFlag) {
            //https://docs.vulkan.org/refpages/latest/refpages/source/VkDescriptorSetVariableDescriptorCountAllocateInfo.html
            VkDescriptorSetVariableDescriptorCountAllocateInfo varBindingInfo{};
            varBindingInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
            uint32_t bindlessVarSizeDescriptorCount = DESCRIPTOR_VAR_COUNT_SIZE;
            varBindingInfo.pDescriptorCounts = &bindlessVarSizeDescriptorCount;
            varBindingInfo.descriptorSetCount = 1;
            allci.pNext = &varBindingInfo;
            vkAllocateDescriptorSets(ctx->device, &allci, &desSet);
        }
        else {
            vkAllocateDescriptorSets(ctx->device, &allci, &desSet);
        }
        block->mInUseNum++;
        block->lastActiveFrame = frame;
        return desSet;
    }

    rs_descriptorSet_vk* DescriptorSetManager::AllocateDescriptorSetFromDedicatePool(uint64_t frame, rs_context_vk* ctx, rs_pipeline* pipeline, uint32_t setIdx)
    {
        auto pipelineLayout = (rs_pipeline_layout_vk*)pipeline->pipelineLayout;
        rs_descriptorset_layout_vk* setlayout = nullptr;
        for (const auto& [idx, layout] : pipelineLayout->setLayouts) {
            if (idx == setIdx) {
                setlayout = layout;
                break;
            }
        }
        //Create a dedicated pool
        auto descriptorPool = createDedicatedPool(ctx, setlayout->bindingHash);
        auto set = tryAllocateFromPool(frame, ctx, descriptorPool, setlayout);
        rs_descriptorSet_vk* ret = new rs_descriptorSet_vk;
        ret->native = set;
        ret->layout = setlayout;
        ret->pool = descriptorPool;

        this->mDedicatedBlocks.insert(
            {
                descriptorPool
            }
        );

        return ret;
    }

    rs_descriptorSet_vk* DescriptorSetManager::AllocateDescriptorSet(uint64_t frame, rs_context_vk* ctx, rs_pipeline* pipeline, uint32_t setIdx)
    {
        auto pipelineLayout = (rs_pipeline_layout_vk*)pipeline->pipelineLayout;
        rs_descriptorset_layout_vk* setlayout = nullptr;
        for (const auto& [idx, layout] : pipelineLayout->setLayouts) {
            if (idx == setIdx) {
                setlayout = layout;
                break;
            }
        }

        if (!setlayout) {
            Log::error("No DescriptorSet { " + std::to_string(setIdx) + " } in pipeline { " + std::to_string((uint64_t)pipeline->pipelineLayout->native) + " }.");
            assert(false);
            return nullptr;
        }

        auto ret = AllocateDescriptorSet(frame, ctx, setlayout);
        
        return ret;
    }

    rs_descriptorSet_vk* DescriptorSetManager::AllocateDescriptorSet(uint64_t frame,rs_context_vk* ctx, rs_descriptorset_layout_vk* rs) {
        int frame_idx = frame % m_maxFrame;
        for (auto&& i : this->m_pools[frame_idx]) {
            auto desSet = tryAllocateFromPool(frame,ctx, i, rs);
            if (desSet != VK_NULL_HANDLE) {
                i->lastActiveFrame = frame;
                rs_descriptorSet_vk* ret = new rs_descriptorSet_vk;
                ret->pool = i;
                ret->native = desSet;
                ret->layout = rs;
                return ret;
            }
        }
        this->m_pools[frame_idx].push_back(createNewPool(ctx));
        auto pool = m_pools[frame_idx].back();
        auto desSet = tryAllocateFromPool(frame,ctx, pool, rs);
        assert(desSet != VK_NULL_HANDLE && "TRY ALLOCATE FROM DEDICATE POOL");
        rs_descriptorSet_vk* ret = new rs_descriptorSet_vk;
        ret->native = desSet;
        ret->layout = rs;
        ret->pool = pool;
        return ret;
    }

    void DescriptorSetManager::ReturnDescriptorSet(rs_context_vk* ctx, rs_descriptorSet_vk*& descriptorSet)
    {
        auto pool = descriptorSet->pool;
        if (pool) {
            //For Descriptor Set Pool, You can only reset entire pool   
            pool->mReturnedNum++;

        }
        else {
            assert(0);
        }
        delete descriptorSet;
    }

    DescriptorPoolBlock* DescriptorSetManager::createNewPool(rs_context_vk* ctx)
    {
        DescriptorPoolBlock* block = new DescriptorPoolBlock;
        block->maxSets = this->m_maxSet;
        block->sizes = this->m_defaultSize;

        VkDescriptorPoolCreateInfo ci{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        ci.                       maxSets = m_maxSet;
        ci.                       poolSizeCount = m_defaultSize.size();
        ci. pPoolSizes = m_defaultSize.data();
        VK_CHECK(vkCreateDescriptorPool(ctx->device, &ci, 0, &block->pool), {
         });

        return block;
    }

    DescriptorPoolBlock* DescriptorSetManager::createDedicatedPool(rs_context_vk* ctx, const rs_vk_descriporset_layout_hash& layoutHash)
    {

        PoolSizeInfo sizes;
        //This will allow descriptorSet to be updated after submit
        VkDescriptorPoolCreateFlags flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT; 
        //Try construct one from 
        for (const auto& descriptor : layoutHash.mDescriptors) {
            VkDescriptorPoolSize size{};
            size.type = toVkDescriptorType(descriptor.type);
            if (descriptor.count == 0) {
                if (layoutHash.DescriptorSetFlags & DescriptorSetVarCountBindingFlag && BindlessAvailable) {
                    size.descriptorCount = DESCRIPTOR_VAR_COUNT_SIZE; //this is the memory that driver will really allocated.
                    //Variable Length in shader.
                    flags |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT;
                }
                else {
                    assert(false);
                    return nullptr;
                }
            }
            sizes.push_back(size);
        }
        DescriptorPoolBlock* block = new DescriptorPoolBlock;
        block->sizes = sizes;
        block->maxSets = 1;
        VkDescriptorPoolCreateInfo ci{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        ci.flags = flags;
        ci.maxSets = 1;
        ci.poolSizeCount = sizes.size();
        ci.pPoolSizes = sizes.data();
        VK_CHECK(vkCreateDescriptorPool(ctx->device, &ci, 0, &block->pool), {

        return nullptr;
            });
        return block;
    }


    std::pair<UniformBufferObject*, uint32_t> DescriptorSetManager::getCurFrameDefaultUBO()
    {
        return {curFrameDefaultUBO,curFrameDyOffset};
    }

    std::pair<UBO*, uint32_t> DescriptorSetManager::getDybuffer(uint64_t frame,uint32_t fif, rs_context_vk* ctx, void* data, uint64_t size, QueueType queue, UniformBufferObject* formerUBO)
    {
        if (formerUBO) {
            returnDybuffer(formerUBO);
            std::lock_guard<std::mutex> lock(mAllocateUniformBufferLock);
            uint32_t offsetPos = 0;
            if (formerUBO->allocateInFrame(data, size, fif, frame, offsetPos)) {
                formerUBO->mInUsedNum.fetch_add(1);
                return { formerUBO,offsetPos };
            }
        }

        UBO* choosenUBO = nullptr;
        uint32_t offsetPos = 0;

        {
            std::lock_guard<std::mutex> lock(mAllocateUniformBufferLock);
            for (auto&& ubo : mUniformBufferLists) {
                if (ubo->mQueue == queue && ubo->allocateInFrame(data, size, fif, frame, offsetPos)) {
                    choosenUBO = ubo;
                    choosenUBO->mInUsedNum.fetch_add(1);
                    break;
                }
            }
        }
        if (!choosenUBO) {
            std::lock_guard<std::mutex> lock(mAllocateUniformBufferLock);
            choosenUBO = createUBO(ctx, RingBufferSize, RingBufferAlignedSize, queue);
            mUniformBufferLists.push_back(choosenUBO);
            if (!choosenUBO->allocateInFrame(data, size, fif, frame, offsetPos)) {
                assert("ERROR");
                return { choosenUBO  , 0};
            }
            else {
                choosenUBO->mInUsedNum.fetch_add(1);
            }
        }
        return { choosenUBO,offsetPos };
    }

    void DescriptorSetManager::returnDybuffer(UniformBufferObject* ubo)
    {
        ubo->mInUsedNum.fetch_sub(1);
    }

    UBO* DescriptorSetManager::createUBO(rs_context_vk* ctx, uint64_t createSize, uint32_t alignedSize, QueueType queue)
    {
        BufferDesc createBufferDesc{};
        uint64_t createSizeNew = (createSize / alignedSize + (createSize % alignedSize > 0 ? 1 : 0) ) * alignedSize;
        createBufferDesc.byteSize = createSizeNew;
        createBufferDesc.bufUsage = BufferType_Uniform;
        createBufferDesc.queueType = queue;
        createBufferDesc.mappable = true;
        auto buffer= createRsBufferVk(ctx, createBufferDesc);
        mapRsBuffer(ctx, buffer);
        UBO* ret = new UBO(ctx->maxFrameInFlight,createSizeNew,alignedSize);
        ret->mLastActiveFrame = 0;
        ret->mQueue = queue;
        ret->mBuffer = buffer;
        return ret;
    }

    void DescriptorSetManager::destroyUBO(rs_context_vk* ctx, UniformBufferObject* ubo)
    {
        rs_buffer_vk* buffer = (rs_buffer_vk*)ubo->mBuffer;
        destroyRsBuffer(ctx, buffer);
        delete ubo;
    }

    rs_pipeline_layout_vk* DescriptorSetManager::createFromShaders(rs_context_vk* ctx, std::vector<rs_shader_module_vk*>& shaders)
    {

        PipelineLayoutInfo setLayouts =
            getPipelineShaderInfo(shaders.data(), shaders.size());

        return createRsPipelineLayout(ctx, setLayouts);
    }

    void DescriptorSetManager::beginFrame(rs_context_vk* ctx, uint64_t frame)
    {  
        curFif = frame % m_maxFrame;
        for (auto i = m_pools[curFif].begin(); i != m_pools[curFif].end(); ) {
            auto poolBlock = (*i);
            if (poolBlock->mReturnedNum == poolBlock->mInUseNum && frame - poolBlock->lastActiveFrame > Max_Vacant_Frame) {
                vkResetDescriptorPool(ctx->device, poolBlock->pool, 0);
                vkDestroyDescriptorPool(ctx->device, poolBlock->pool, 0);
                delete poolBlock;
                i = m_pools[curFif].erase(i);
            }
            else {
                ++i;
            }
        }

        for (auto it = mDedicatedBlocks.begin(); it != mDedicatedBlocks.end();) {
            auto poolBlock = (*it);
            if (poolBlock->mInUseNum == poolBlock->mInUseNum) {
                vkResetDescriptorPool(ctx->device, poolBlock->pool, 0);
                vkDestroyDescriptorPool(ctx->device, poolBlock->pool, 0);
                delete poolBlock;
                it = mDedicatedBlocks.erase(it);
            }
            else {
                it++;
            }
        }

        {
            std::lock_guard<std::mutex> lock(mAllocateUniformBufferLock);
            for (auto i = this->mUniformBufferLists.begin(); i != mUniformBufferLists.end(); ) {
                auto UBO = *i;
                UBO->releaseFrame(curFif);
                if (UBO->mInUsedNum == 0 && frame - UBO->mLastActiveFrame >= Max_Vacant_Frame) {
                    destroyUBO(ctx, UBO);
                    i = mUniformBufferLists.erase(i);
                }
                else {
                    i++;
                }
            }
        }

		uint64_t data = 0xCDCDCDCDCDCDCDCD;
		auto pair = getDybuffer(frame, frame % ctx->maxFrameInFlight, ctx, &data, 8, QueueType::QueueType_Graphics,nullptr);
        //return it immediately
        returnDybuffer(pair.first);
        curFrameDefaultUBO = pair.first;
		curFrameDyOffset = pair.second;
    }

    void DescriptorSetManager::endFrame(rs_context_vk* ctx, uint64_t frame)
    {

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
					destroyDescriptorSetLayout(ctx, rs);
				}
            }
            else {
                assert(0 && "Not Managed set layout");
            }


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
        auto layoutHashCopy = layoutHash;
        Name nameSubstitute = Name("Invalid");
        auto& bindings = layoutHashCopy.mDescriptors;
        ci.bindingCount = bindings.size();
        std::vector< VkDescriptorSetLayoutBinding> vkBindings;
        vkBindings.reserve(bindings.size());

        for (auto&& binding : bindings) {
            //Invalid origin name, we don't care about name in set layout.
            binding.bindingItemName = nameSubstitute;
            //Use 'count' to mark void position. 
            if (binding.type == UniformType::Count)continue;
            VkDescriptorSetLayoutBinding b{};
            auto vkBinding = toVkBindingPos(binding.bindingPos);
            b.binding = vkBinding.bindingIdx;
            b.descriptorCount = binding.count;
            b.descriptorType = toVkDescriptorType(binding.type);
            b.stageFlags = toVkShaderStageFlags(binding.shaderVisibleStage);
            b.pImmutableSamplers = 0;
            vkBindings.push_back(b);
        }

        //IMPORTANT!!!!
        ci.pBindings = vkBindings.data();
        ci.bindingCount = vkBindings.size();
        
        VkDescriptorSetLayout layout;
        VK_CHECK(vkCreateDescriptorSetLayout(ctx->device, &ci, 0, &layout), {
            return nullptr;
        });
        rs_descriptorset_layout_vk* l = new rs_descriptorset_layout_vk;
        l->native = layout;

        {
            std::lock_guard<std::mutex> lock(mMutex);
            auto itor = mLayoutMap.find(layoutHashCopy);
            if (itor != mLayoutMap.end()) {
                destroyDescriptorSetLayout(ctx, l);
                itor->second->accquir();
                return itor->second;
            }
            else {
                l->bindingHash = layoutHashCopy;
                this->mLayoutMap.insert({
                    layoutHashCopy,l
                    });
                l->accquir();
            }
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

	UniformBufferObject::UniformBufferObject(uint32_t maxFrameInFlight, uint32_t bufferSize, uint32_t alignedSize)
        :mRingBufferAllocator(bufferSize, alignedSize,true,true),mFrameAllocateInfo(maxFrameInFlight),mAlignment(alignedSize)
    {
        
    }
    void UniformBufferObject::releaseFrame(uint32_t frameInFlight)
    {
        auto& FrameAllocInfo = mFrameAllocateInfo[frameInFlight];
        auto tail = mRingBufferAllocator.tail_offset();
        auto head = mRingBufferAllocator.head_offset();
        if (FrameAllocInfo.totalAdvanceSize != 0) {
            mRingBufferAllocator.release(FrameAllocInfo.totalAdvanceSize);
        }
        FrameAllocInfo = { };
    }
    bool UniformBufferObject::allocateInFrame(void* data, uint32_t size, uint32_t frameInFlight, uint64_t frame, uint32_t& offsetInBuffer)
    {

        auto reservation = mRingBufferAllocator.reserve_and_commit(size);
        if (reservation.count != 1) return false;
        auto& FrameAllocInfo = mFrameAllocateInfo[frameInFlight];
        offsetInBuffer = reservation.parts[0].offset;
        if (FrameAllocInfo.totalAdvanceSize == 0) {
            FrameAllocInfo.headOffset = offsetInBuffer;
            mLastActiveFrame = frame;
        }
        //NOTICE: ITS OK HERE, SINCE WE CAN ASSURE THIS BUFFER IS NOT TOUCHED BY GPU NOW
        mBuffer->state = ResourceState::HostWrite;
        uint8_t* dataPtr = (uint8_t*)mBuffer->mappedPtr;
        memcpy(dataPtr + offsetInBuffer, (uint8_t*)data,size);
		mFrameAllocateInfo[frameInFlight].totalAdvanceSize += reservation.total_advance;

		return true;
    }
}