#include "vulkan/vulkan_descriptor_set.h"
#include "vulkan/vulkan_render_function.h"
#include "render_log.h"
#include <vulkan/vulkan_pipeline.h>
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
            //TODO: change to dynamic
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
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

    DescriptorSetManager::DescriptorSetManager(int maxFrame)
    {
        m_maxFrame = maxFrame;
        this->m_pools.resize(maxFrame);
        this->m_frameBuffers.resize(maxFrame);
        VkDescriptorPoolSize poolFactor{};

        poolFactor.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
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

        poolFactor.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        poolFactor.descriptorCount = 10;
        this->m_defaultSize.push_back(poolFactor);

    }

    VkDescriptorSet DescriptorSetManager::tryAllocateFromPool(uint64_t frame,rs_context_vk* ctx, DescriptorPoolBlock& block, rs_descriptorset_layout_vk* layout) {
        auto& type = layout->bindingHash.mDescriptors;

        block. lastActiveFrame = frame;
        if (block.maxSets == 0)
            return VK_NULL_HANDLE;

        for (auto i = 0; i < (int)ResourceType::Count; ++i) {
            if (block.sizes[i].descriptorCount < layout->bindingHash.mAllocaHint.hint[i]) {
                return VK_NULL_HANDLE;
            }
        }

        for (auto i = 0; i < (int)ResourceType::Count; ++i) {
            block.sizes[i].descriptorCount -= layout->bindingHash.mAllocaHint.hint[i];
        }
        block.maxSets--;

        VkDescriptorSetAllocateInfo allci{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allci.                descriptorPool = block.pool;
        allci.                        descriptorSetCount = 1;
        VkDescriptorSetLayout _layout = (VkDescriptorSetLayout)layout->native;
        allci.pSetLayouts = &_layout;
        VkDescriptorSet desSet;
        vkAllocateDescriptorSets(ctx->device, &allci, &desSet);
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

        return AllocateDescriptorSet(frame, ctx, setlayout);
    }

    rs_descriptorSet_vk* DescriptorSetManager::AllocateDescriptorSet(uint64_t frame,rs_context_vk* ctx, rs_descriptorset_layout_vk* rs) {
        int frame_idx = frame % m_maxFrame;
        for (auto&& i : this->m_pools[frame_idx]) {
            auto desSet = tryAllocateFromPool(frame,ctx, i, rs);
            if (desSet != VK_NULL_HANDLE) {
                i.lastActiveFrame = 0;
                rs_descriptorSet_vk* ret = new rs_descriptorSet_vk;
                ret->native = desSet;
                ret->layout = rs;
                return ret;
            }
        }
        this->m_pools[frame_idx].push_back(createNewPool(ctx));
        auto& pool = m_pools[frame_idx].back();
        auto desSet = tryAllocateFromPool(frame,ctx, pool, rs);
        assert(desSet != VK_NULL_HANDLE);
        rs_descriptorSet_vk* ret = new rs_descriptorSet_vk;
        ret->native = desSet;
        ret->layout = rs;
        return ret;
    }

    void DescriptorSetManager::ReturnDescriptorSet(uint64_t frame, rs_context_vk* ctx, rs_descriptorSet_vk*& descriptorSet)
    {
        delete descriptorSet;
        descriptorSet = 0;
    }

    DescriptorPoolBlock DescriptorSetManager::createNewPool(rs_context_vk* ctx)
    {
        DescriptorPoolBlock block{};
        block.maxSets = this->m_maxSet;
        block.sizes = this->m_defaultSize;

        VkDescriptorPoolCreateInfo ci{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        ci.                       maxSets = m_maxSet;
        ci.                       poolSizeCount = m_defaultPoolAllocSize.size();
        ci. pPoolSizes = m_defaultPoolAllocSize.data();
        VK_CHECK(vkCreateDescriptorPool(ctx->device, &ci, 0, &block.pool), {
         });

        return block;
    }

    DescriptorSetManager::dyUBuffer DescriptorSetManager::createNewDyBuffer(rs_context_vk* ctx, int size, uint8_t queueType)
    {
        BufferDesc desc{};
        desc.byteSize = (uint32_t)size;
        desc.bufUsage = BufferType_Uniform;
        desc.mappable = true;
        desc.queueType = queueType;
        auto rsBuffer = createRsBuffer(ctx, desc);
        dyUBuffer dy{};
        dy.buffer = rsBuffer;
        dy.freeSize = size;
        dy.maxSize  = size;
        dy.queueType = queueType;
        return dy;
    }

    void DescriptorSetManager::updateBufferData(uint64_t frame, rs_context_vk* ctx, rs_descriptorSet_vk* descriptorSet, int binding, void* data, int size, uint8_t queueType)
    {
        if (descriptorSet->layout->bindingHash.mDescriptors.size() <= binding) {
            return;
        }
        auto& bindingInfo = descriptorSet->layout->bindingHash.mDescriptors[binding];
        if (bindingInfo.type != ResourceType::UniformBuffer) {
            assert(0 && "Wrong binding type");
            return;
        }
        auto curFif = frame % ctx->maxFrameInFlight;
        auto& dyBuffersFrame = m_frameBuffers[curFif];
        dyUBuffer* targetBuffer = 0;
        auto itor = dyBuffersFrame.begin();
        while (itor != dyBuffersFrame.end()) {
            auto& buffer = *itor;
            if (buffer.queueType == queueType && buffer.freeSize > size) {
                targetBuffer = &buffer;
                break;
            }
            ++itor;
        }
        if (!targetBuffer) {
            auto newBuffer = createNewDyBuffer(
                ctx, Max_Uniform_Buffer_Block_Size, queueType
            );
            dyBuffersFrame.push_back(
                newBuffer
            );

            auto it = --dyBuffersFrame.end();
            targetBuffer = &(*it);
        }
        VkWriteDescriptorSet info{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

        VkDescriptorBufferInfo binfo{};
        binfo.        buffer = (VkBuffer)targetBuffer->buffer->native;
        binfo.        offset = targetBuffer->maxSize - targetBuffer->freeSize;
        binfo.        range  = size;
        info.                  dstSet = (VkDescriptorSet)descriptorSet->native;
        info.                  dstBinding = binding;
        info.                  dstArrayElement = 0;
        info.                  descriptorCount = bindingInfo.count;
        info.                  descriptorType = toVkDescriptorType(bindingInfo.type);
        info.                  pBufferInfo = &binfo;
        vkUpdateDescriptorSets(
            ctx->device, 1, &info, 0, 0
        );
        if (!targetBuffer->buffer->mappedPtr) {
            targetBuffer->buffer->mappedPtr = mapRsBuffer(ctx, targetBuffer->buffer);
        }

        auto mapPtr = targetBuffer->buffer->mappedPtr;
        memcpy((char*)mapPtr + targetBuffer->maxSize - targetBuffer->freeSize, data, size);
        targetBuffer->lastActiveFrame = frame;
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
        if (bindingInfo.type != ResourceType::StorageBuffer) {
            assert(0 && "Wrong binding type");
            return;
        }

        VkWriteDescriptorSet writeSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        writeSet.dstBinding = binding;
        writeSet.dstArrayElement = 0;
        writeSet.descriptorCount = 1;
        writeSet.descriptorType = toVkDescriptorType(bindingInfo.type);
        VkDescriptorBufferInfo bInfo{};
        bInfo.buffer = (VkBuffer)buffer->native;
        bInfo.offset = 0;
        bInfo.range = buffer->byteSize;
        writeSet.pBufferInfo = &bInfo;

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
            vkResetDescriptorPool(ctx->device, i->pool, 0);
            if (i->lastActiveFrame - frame >= Max_Vacant_Frame) {
                vkDestroyDescriptorPool(ctx->device, i->pool, 0);
                i = m_pools[curframeIdx].erase(i);
            }
            else {
                ++i;
            }
        }

        for (auto i = this->m_frameBuffers[curframeIdx].begin(); i != m_frameBuffers[curframeIdx].end(); ) {
            i->freeSize = i->maxSize;
            if (i->lastActiveFrame - frame >= Max_Vacant_Frame) {
                destroyRsBuffer(ctx, i->buffer);
                i = m_frameBuffers[curframeIdx].erase(i);
            }
            else {
                ++i;
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