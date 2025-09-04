#include "vulkan/vulkan_descriptor_set.h"
#include "vulkan/vulkan_render_function.h"
#include "render_log.h"
#include <vulkan/vulkan_pipeline.h>
#include <map>
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
        //case ResourceType::AccelerationStructure:
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

        poolFactor.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        poolFactor.descriptorCount = 10;
        this->m_defaultSize.push_back(poolFactor);

        poolFactor.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolFactor.descriptorCount = 10;
        this->m_defaultSize.push_back(poolFactor);

        poolFactor.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        poolFactor.descriptorCount = 10;
        this->m_defaultSize.push_back(poolFactor);

        poolFactor.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        poolFactor.descriptorCount = 10;
        this->m_defaultSize.push_back(poolFactor);

        poolFactor.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        poolFactor.descriptorCount = 10;
        this->m_defaultSize.push_back(poolFactor);

        poolFactor.type = VK_DESCRIPTOR_TYPE_SAMPLER;
        poolFactor.descriptorCount = 10;
        this->m_defaultSize.push_back(poolFactor);

        //poolFactor.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        //poolFactor.descriptorCount = 10;
        //this->m_defaultSize.push_back(poolFactor);

    }

    VkDescriptorSet DescriptorSetManager::tryAllocateFromPool(uint64_t frame,rs_context_vk* ctx, DescriptorPoolBlock* block, rs_descriptorset_layout_vk* layout) {
        auto& type = layout->bindingHash.mDescriptors;

        if (block->maxSets == 0)
            return VK_NULL_HANDLE;

        for (auto i = 0; i < (int)ResourceType::Count; ++i) {
            if (block->sizes[i].descriptorCount < layout->bindingHash.mAllocaHint.hint[i]) {
                return VK_NULL_HANDLE;
            }
        }

        for (auto i = 0; i < (int)ResourceType::Count; ++i) {
            block->sizes[i].descriptorCount -= layout->bindingHash.mAllocaHint.hint[i];
        }
        block->maxSets--;

        VkDescriptorSetAllocateInfo allci{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allci.                descriptorPool = block->pool;
        allci.                        descriptorSetCount = 1;
        VkDescriptorSetLayout _layout = (VkDescriptorSetLayout)layout->native;
        allci.pSetLayouts = &_layout;
        VkDescriptorSet desSet;
        vkAllocateDescriptorSets(ctx->device, &allci, &desSet);
        block->mInUseNum++;
        block->lastActiveFrame = frame;
        return desSet;
    }

    rs_descriptorSet_vk* DescriptorSetManager::AllocateDescriptorSet(uint64_t frame, rs_context_vk* ctx, rs_pipeline_vk* pipeline, uint32_t setIdx)
    {
        auto pipelineLayout = pipeline->layout;
        rs_descriptorset_layout_vk* setlayout = nullptr;
        for (const auto& [idx, layout] : pipelineLayout->setLayouts) {
            if (idx == setIdx) {
                setlayout = layout;
            }
        }

        if (!setlayout) {
            assert(0);
        }

        auto ret = AllocateDescriptorSet(frame, ctx, setlayout);
        
        rs_binding_data binding{};
        binding.type = ResourceType::Count;
        for (auto& bindingSlot : setlayout->bindingHash.mDescriptors) {
            ret->mBindingData.resize(bindingSlot.binding + 1, binding);
            ret->mBindingData[bindingSlot.binding].type = bindingSlot.type;
        }
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
        assert(desSet != VK_NULL_HANDLE);
        rs_descriptorSet_vk* ret = new rs_descriptorSet_vk;
        ret->native = desSet;
        ret->layout = rs;
        ret->pool = pool;
        return ret;
    }

    void DescriptorSetManager::ReturnDescriptorSet(uint64_t frame, rs_context_vk* ctx, rs_descriptorSet_vk*& descriptorSet)
    {
        if (descriptorSet->pool) {
            //For Descriptor Set Pool, You can only reset entire pool   
            descriptorSet->pool->mReturnedNum++;
        }
        else {
            assert(0);
        }
        for (const rs_binding_data& bindingData : descriptorSet->mBindingData) {
            switch (bindingData.type)
            {
            case Render::ResourceType::UniformBuffer:
            {
                UniformBufferObject* ubo = (UBO*)bindingData.native;
                if (ubo) {
                    ubo->mUsingNum--;
                }
            }
            break;
            case      Render::ResourceType::StorageBuffer:
            case       Render::ResourceType::StorageImage:
            case      Render::ResourceType::Texture:
            case        Render::ResourceType::InputAttachment:
            case  Render::ResourceType::Sampler:
            //case Render::ResourceType::AccelerationStructure:
            default:
                break;
            }
        
        }
        delete descriptorSet;
        descriptorSet = 0;
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


    UBO* DescriptorSetManager::createUBO(rs_context_vk* ctx, uint64_t createSize, uint32_t alignedSize, QueueType queue)
    {
        BufferDesc createBufferDesc{};
        uint64_t createSizeNew = (createSize / alignedSize + (createSize % alignedSize > 0 ? 1 : 0) ) * alignedSize;
        createBufferDesc.byteSize = createSizeNew;
        createBufferDesc.bufUsage = BufferType_Uniform;
        createBufferDesc.queueType = queue;
        createBufferDesc.mappable = true;
        auto buffer= createRsBuffer(ctx, createBufferDesc);
        UBO* ret = new UBO(ctx->maxFrameInFlight);
        ret->mLastActive = 0;
        ret->mAlignedSize = alignedSize;
        ret->native = buffer;
        ret->queue = queue;
        ret->mMaxSize = createSizeNew;
        mapRsBuffer(ctx, buffer);
        return ret;
    }

    void DescriptorSetManager::updateDynamicBuffer(uint64_t frame, rs_context_vk* ctx, rs_descriptorSet_vk* descriptorSet, int binding, void* data, uint64_t size,QueueType queue)
    {

        {
        //Check DescriptorSet
            const auto& layoutData =
                descriptorSet->layout->bindingHash.mDescriptors;
            
            if (layoutData.size() <= binding|| layoutData[binding].type != ResourceType::UniformBuffer || layoutData[binding].size < size) {
                assert(0 && "mismatch layout");
                return;
            }

            if (layoutData[binding].count > 1) {
                assert(0 && "For UBO, descriptor count must equal to 1.");
                return;
            }

            if (descriptorSet->mBindingData[binding].type == ResourceType::Count) {
                descriptorSet->mBindingData[binding].type = ResourceType::UniformBuffer;
            }
            else if (descriptorSet->mBindingData[binding].type != ResourceType::UniformBuffer) {
                assert(0 && "mismatch layout");
                return;
            }
        }

        auto fif = frame % ctx->maxFrameInFlight;
        UBO* choosenUBO = nullptr;
        uint32_t offsetPos = 0;
        auto& bindingSlot = descriptorSet->mBindingData[binding];
        //Use former uniform buffer bind in descriptor set.
        if (descriptorSet->mBindingData[binding].native) {
            std::lock_guard<std::mutex> lock(mAllocateUniformBufferLock);
            choosenUBO = (UBO*)bindingSlot.native;
            if (!choosenUBO->pushBytes(data, size, fif, frame, offsetPos)) {
                choosenUBO->mUsingNum--;
                choosenUBO = 0;
            }
        }

        if(!choosenUBO){
            std::lock_guard<std::mutex> lock(mAllocateUniformBufferLock);
            for (auto&& ubo : mUniformBufferLists) {
                if (ubo->queue == queue && ubo->pushBytes(data, size, fif, frame, offsetPos)) {
                    choosenUBO = ubo;
                    choosenUBO->mUsingNum++;
                    break;
                }
            }
        }
        if (!choosenUBO) {
            std::lock_guard<std::mutex> lock(mAllocateUniformBufferLock);
            choosenUBO = createUBO(ctx, RingBufferSize, RingBufferAlignedSize,queue);
            mUniformBufferLists.push_back(choosenUBO);
            if (!choosenUBO->pushBytes(data, size, fif, frame, offsetPos)) {
                assert("ERROR");
                return;
            }
            else {
                choosenUBO->mUsingNum++;
            }
        }
        if (choosenUBO != bindingSlot.native) {
            updateBufferBind(frame, ctx, descriptorSet, binding, (rs_buffer_vk*)choosenUBO->native);
        }
        bindingSlot.native = choosenUBO;
        bindingSlot.uboDyOffset = offsetPos;
    }

    void DescriptorSetManager::updateBufferData(uint64_t frame, rs_context_vk* ctx, rs_descriptorSet_vk* descriptorSet, int binding, void* data, int size, QueueType queueType)
    {
        updateDynamicBuffer(frame, ctx, descriptorSet, binding, data, size, queueType);
    }
    void DescriptorSetManager::updateSampler(uint64_t frame, rs_context_vk* ctx, rs_descriptorSet_vk* descriptorSet, int binding, rs_sampler_vk* sampler, uint8_t queueType) {
        if (descriptorSet->layout->bindingHash.mDescriptors.size() <= binding) {
            return;
        }
        auto& bindingInfo = descriptorSet->layout->bindingHash.mDescriptors[binding];
        if (bindingInfo.type != ResourceType::Sampler) {
            assert(0 && "Wrong binding type");
            return;
        }

        VkWriteDescriptorSet writeSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        writeSet.dstBinding = binding;
        writeSet.dstArrayElement = 0;
        writeSet.descriptorCount = 1;
        writeSet.descriptorType = toVkDescriptorType(bindingInfo.type);
        VkDescriptorImageInfo iInfo{};
        iInfo.sampler = (VkSampler)sampler->native;
        writeSet.pImageInfo = &iInfo;

        vkUpdateDescriptorSets(ctx->device, 1, &writeSet, 0, 0);

    }

    rs_pipeline_layout_vk* DescriptorSetManager::createFromShaders(rs_context_vk* ctx, std::vector<rs_shader_module_vk*>& shaders)
    {
        std::vector<rs_vk_descriporset_layout_hash> hash;
        std::map<int, std::map<int,BindingInfo>> setsBindings;
        for (auto&&shader: shaders) {
            for (const auto& set : shader->reflectInfo) {
                for (const auto& binding : set.mInfo) {
                    auto& sets = setsBindings[set.setIdx];
                    auto itor = sets.find(binding.binding);
                    if (itor != sets.end()) {
                        BindingInfo info = binding;
                        info. shaderVisibleStage = (uint16_t)shader->shaderStage; //shader stage
                        sets.insert({ binding.binding,info });
                    }
                    else {
                        auto& curInfo = itor->second;
                        if (curInfo.count == binding.count &&
                            curInfo.size == binding.size &&
                            curInfo.type == binding.type
                            ) {
                            curInfo.shaderVisibleStage |= (uint16_t)shader->shaderStage;
                        }
                        else {
                            Log::error("MisMatch duplicate Descriptor in Shader: " + shader->shaderName + " set: " + std::to_string(set.setIdx) + " binding: " + std::to_string(binding.binding));
                            continue;
                        }
                    
                    }
                }
            }
        }

        std::vector< DescritporSetInfo> setLayouts;
        for (auto&& [setId, set] : setsBindings) {
            hash.push_back({});
            auto& tar = hash[hash.size() - 1];
            for (auto&& binding : set) {
                tar.mDescriptors.push_back(binding.second);
            }
            tar.init();
            setLayouts.push_back({ setId,tar });
            assert(tar.checkValid() == true);
        }
        return createRsPipelineLayout(ctx, setLayouts);
    }

    void DescriptorSetManager::updateImage(uint64_t frame, rs_context_vk* ctx, rs_descriptorSet_vk* descriptorSet, int binding, rs_image_vk* image, uint8_t queueType)
    {
        if (descriptorSet->layout->bindingHash.mDescriptors.size() <= binding) {
            return;
        }
        auto& bindingInfo = descriptorSet->layout->bindingHash.mDescriptors[binding];
        if (bindingInfo.type != ResourceType::StorageBuffer) {
            assert(0 && "Wrong binding type");
            return;
        }

        VkWriteDescriptorSet writeSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        writeSet.dstBinding = binding;
        writeSet.dstArrayElement = 0;
        writeSet.descriptorCount = 1;
        writeSet.descriptorType = toVkDescriptorType(bindingInfo.type);
        VkDescriptorImageInfo iInfo{};
        iInfo.imageView = (VkImageView)image->view;
        iInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        writeSet.pImageInfo = &iInfo;

        vkUpdateDescriptorSets(ctx->device, 1, &writeSet, 0, 0);

    }

    void DescriptorSetManager::updateBufferBind(uint64_t frame, rs_context_vk* ctx, rs_descriptorSet_vk* descriptorSet, int binding, rs_buffer_vk* buffer)
    {
        if (descriptorSet->layout->bindingHash.mDescriptors.size() <= binding) {
            return;
        }
        auto& bindingInfo = descriptorSet->layout->bindingHash.mDescriptors[binding];

        VkWriteDescriptorSet writeSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        writeSet.dstBinding = binding;
        writeSet.dstArrayElement = 0;
        writeSet.descriptorCount = 1;
        writeSet.descriptorType = toVkDescriptorType(bindingInfo.type);
        VkDescriptorBufferInfo bInfo{};
        bInfo.buffer = (VkBuffer)buffer->native;
        bInfo.offset = 0;
        if (bindingInfo.type == ResourceType::UniformBuffer) {
            bInfo.range = bindingInfo.size;
        }
        else {
            bInfo.range = buffer->byteSize;
        }
        writeSet.pBufferInfo = &bInfo;
        writeSet.dstSet = (VkDescriptorSet)descriptorSet->native;
        vkUpdateDescriptorSets(ctx->device, 1, &writeSet, 0, 0);
    }

    void DescriptorSetManager::updateBuffer(uint64_t frame, rs_context_vk* ctx, rs_descriptorSet_vk* descriptorSet, int binding, rs_buffer_vk* buffer, uint8_t queueType)
    {
        if (descriptorSet->layout->bindingHash.mDescriptors.size() <= binding) {
            return;
        }
        auto& bindingInfo = descriptorSet->layout->bindingHash.mDescriptors[binding];
        if (bindingInfo.type != ResourceType::StorageBuffer) {
            assert(0 && "Wrong binding type");
            return;
        }

        VkWriteDescriptorSet writeSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        writeSet.                         dstBinding = binding;
        writeSet.                         dstArrayElement = 0;
        writeSet.                         descriptorCount = 1;
        writeSet.                         descriptorType = toVkDescriptorType(bindingInfo.type);
        VkDescriptorBufferInfo bInfo{};
        bInfo.buffer = (VkBuffer)buffer->native;
        bInfo.offset = 0;
        bInfo.range = buffer->byteSize;
        writeSet.pBufferInfo = &bInfo;

        vkUpdateDescriptorSets(ctx->device, 1, &writeSet, 0, 0);

    }

    void DescriptorSetManager::beginFrame(rs_context_vk* ctx, uint64_t frame)
    {  
        int curframeIdx = frame % m_maxFrame;
        for (auto i = m_pools[curframeIdx].begin(); i != m_pools[curframeIdx].end(); ) {
            auto poolBlock = (*i);
            vkResetDescriptorPool(ctx->device, poolBlock->pool, 0);
            if (poolBlock->mReturnedNum == poolBlock->mInUseNum && frame - poolBlock->lastActiveFrame > Max_Vacant_Frame) {
                vkDestroyDescriptorPool(ctx->device, poolBlock->pool, 0);
                delete poolBlock;
                i = m_pools[curframeIdx].erase(i);
            }
            else {
                ++i;
            }
        }

        {
            std::lock_guard<std::mutex> lock(mAllocateUniformBufferLock);
            for (auto i = this->mUniformBufferLists.begin(); i != mUniformBufferLists.end(); ) {
                auto UBO = *i;
                UBO->releaseFrameBytes(curframeIdx);
                if (UBO->mUsingNum == 0 && frame - UBO->mLastActive >= Max_Vacant_Frame) {
                    rs_buffer_vk* buffer = (rs_buffer_vk*)UBO->native;
                    destroyRsBuffer(ctx, buffer);
                    delete UBO;
                    i = mUniformBufferLists.erase(i);
                }
                else {
                    i++;
                }
            }
        }
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