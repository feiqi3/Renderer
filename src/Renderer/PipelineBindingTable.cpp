#include "Renderer/PipelineBindingTable.h"
#include "Renderer/RenderSystem.h"
#include <cstring> 

namespace Render {
    namespace EngineBindlessAPI {
        static uint32_t GetGlobalSamplerIndex(rs_sampler* sampler) {
            if (sampler->bindlessIndex != INVALID_BINDLESS_INDEX) {
                return sampler->bindlessIndex;
            }
            auto bindlessData = RenderSystem::instance()->getGlobalBindlessData();
            assert(bindlessData != nullptr);
            return RenderSystem::instance()->updateGlobalBindlessDataSampler(RenderSystem::instance()->getGlobalBindlessData(), sampler);
        }
        static void UnbindGlobalSampler(uint32_t index) {
            return RenderSystem::instance()->unbindGlobalBindlessDataSampler(RenderSystem::instance()->getGlobalBindlessData(), index);
        }

        static uint32_t GetGlobalTextureIndex(rs_image_view* img) {
            if (img->bindlessIndex != INVALID_BINDLESS_INDEX) {
                return img->bindlessIndex;
            }
            return RenderSystem::instance()->updateGlobalBindlessDataTexture(RenderSystem::instance()->getGlobalBindlessData(), img);
        }
        static uint32_t GetGlobalRWTextureIndex(rs_image_view* img) {
            if (img->bindlessIndex != INVALID_BINDLESS_INDEX) {
                return img->bindlessIndex;
            }
            return RenderSystem::instance()->updateGlobalBindlessDataRWTexture(RenderSystem::instance()->getGlobalBindlessData(), img);
        }

        static void UnbindGlobalTexture(uint32_t index) {
        
            return RenderSystem::instance()->unbindGlobalBindlessDataSampler(RenderSystem::instance()->getGlobalBindlessData(), index);
        }
        static void UnbindGlobalRWTexture(uint32_t index) {

            return RenderSystem::instance()->unbindGlobalBindlessDataRWTexture(RenderSystem::instance()->getGlobalBindlessData(), index);
        }
        // Datasize: 4 (Array Offset) or 8 (BDA)
        //Use null to get data size
        static uint64_t GetBlockBufferBindingData(rs_buffer* buffer, uint32_t& datasize) {
            return RenderSystem::instance()->updateGlobalBindlessDataBuffer(RenderSystem::instance()->getGlobalBindlessData(), buffer,datasize);
        }
        static void UnbindGlobalBuffer(uint64_t data) {
            return RenderSystem::instance()->unbindGlobalBindlessDataBuffer(RenderSystem::instance()->getGlobalBindlessData(), data);

        }
    }
}

namespace Render {

    PipelineBindingTable* MaterialBindingTable::getPipelineBindingTable(const Name& passName)
    {
        for (auto& [name, table] : this->mTables) {
            if (name == passName) {
                return &table;
            }
        }
        return nullptr;
    }
    PipelineBindingTable* MaterialBindingTable::getPipelineBindingTable(rs_pipeline* pipeline) {
        Name findName = Name::Empty();
        for (const auto& [_pipeline, name] : mPipelineToName) {
            if (pipeline == _pipeline) {
                findName = name;
                break;
            }
        }
        if (findName == Name::Empty())return nullptr;
        return getPipelineBindingTable(findName);
    }

    PipelineBindingTable::~PipelineBindingTable() {
        for (auto& bItem : mBindlessItems) {
            for (int i = 0; i < bItem.keepAliveRefs.size(); ++i) {
                auto& var = bItem.keepAliveRefs[i];

                if (var.getBufferPair()) {
                    uint32_t ds = 0;
                    //Get use DBA(8B) / Buffer Array(4B)
                    EngineBindlessAPI::GetBlockBufferBindingData(nullptr, ds);
                    uint32_t slotStride = (ds == 8) ? 2 : 1;

                    //Get old binding pos.
                    if ((i * slotStride) < bItem.bindlessData.size()) {
                        uint64_t oldData = 0;
                        if (ds == 8) {
                            std::memcpy(&oldData, &bItem.bindlessData[i * slotStride], 8);
                        }
                        else {
                            oldData = bItem.bindlessData[i * slotStride];
                        }
                        if (oldData != 0) EngineBindlessAPI::UnbindGlobalBuffer(oldData);
                    }
                }
                else if (var.getTexture()) {
                    if (i < bItem.bindlessData.size()) {
                        uint32_t oldData = bItem.bindlessData[i];
                        if (oldData != 0) EngineBindlessAPI::UnbindGlobalTexture(oldData);
                    }
                }
                else if (var.getSampler()) {
                    if (i < bItem.bindlessData.size()) {
                        uint32_t oldData = bItem.bindlessData[i];
                        if (oldData != 0) EngineBindlessAPI::UnbindGlobalSampler(oldData);
                    }
                }
            }
        }
    }

    void PipelineBindingTable::commit(rs_pipeline* pipeline, rs_drawdata* drawdata)
    {
        auto sys = RenderSystem::instance()->instance();
        for (auto& pp : mBindingSlots) {


            for (int i = 0; i < pp.varArr.size(); ++i) {
                auto& var = pp.varArr[i];

                switch (pp.location.descriptorInfo.type) {
                case UniformType::StorageBuffer:
                case UniformType::ConstantBuffer:
                {
                    if (!var.isBuffer())break;
                    auto bufferPair = var.getBufferPair();
                    sys->updateUniform(pp.location.bindingPos, i, bufferPair->buffer, pipeline, drawdata, bufferPair->offset, bufferPair->size);
                    break;
                }
                case UniformType::UniformBuffer:
                {
                    //TODO: ubo idx
                    void* dataptr = nullptr;
                    uint32_t size = 0;
                    var.getData(&dataptr, &size);
                    if (dataptr && size > 0) {
                        sys->updateUniformBufferData(pp.location.bindingPos, dataptr, size, pipeline,drawdata);
                    }
                    break;
                }

                case UniformType::StorageImage:
                case UniformType::Texture:
                case UniformType::InputAttachment:
                {
                    if (var.isTextureView()) {
                        auto viewPair = var.getTextureView();
                        sys->updateUniform(pp.location.bindingPos, i, viewPair->view, pipeline, drawdata);
                    }else if(var.isTexture()) {
                        sys->updateUniform(pp.location.bindingPos, i, var.getTexture()->getRsImage(), pipeline, drawdata);
                    }
                    else if (var.isTextureView()) {
                        sys->updateUniform(pp.location.bindingPos, i, var.getTextureView()->view, pipeline, drawdata);
                    }
                    break;
                }

                case UniformType::Sampler:
                    sys->updateUniform(pp.location.bindingPos, i, var.getSampler()->getRsSampler(), pipeline, drawdata);
                    break;

                default:
                    break;
                }
            }

        }
    }

    void PipelineBindingTable::init(const Name& passName, rs_pipeline* pipeline) {
        if (!pipeline) return;

        mPassName = passName;
        mPipeline = pipeline;

        mBindingSlots.clear();
        mBindlessItems.clear();
        mName2BindingSlot.clear();
        mName2BindlessSlot.clear();
        mBindingPos2BindingSlot.clear();

        for (const auto& loc : pipeline->resources) {
            if (loc.type == ResourceLocationType::BindingSlot) {
                _ParameterPair pp;
                pp.location = loc;
                pp.parameterImageType = ImageType::Invalid;
                pp.varArr.resize(loc.descriptorInfo.count);

                if (loc.descriptorInfo.type == UniformType::UniformBuffer) {
                    pp.varArr[0].setUniformBuffer(nullptr, loc.descriptorInfo.size);
                }

                mBindingSlots.push_back(std::move(pp));
                uint32_t slotIdx = static_cast<uint32_t>(mBindingSlots.size() - 1);

                mName2BindingSlot[loc.itemName] = slotIdx;
                mBindingPos2BindingSlot[loc.bindingPos] = slotIdx;
            }
        }

        for (const auto& loc : pipeline->resources) {
            if (loc.type == ResourceLocationType::BindlessSlot) {
                _BindlessItem bindlessItem;
                bindlessItem.location = loc;
                bindlessItem.keepAliveRefs.resize(loc.bindlessInfo.count);
                bindlessItem.isUAV = loc.bindlessInfo.isUAV;
                uint32_t realUsedSlots = loc.bindlessInfo.count;
                bindlessItem.bindlessData.resize(loc.bindlessInfo.count, INVALID_BINDLESS_INDEX);
                mBindlessItems.push_back(std::move(bindlessItem));
                mName2BindlessSlot[loc.itemName] = static_cast<uint32_t>(mBindlessItems.size() - 1);
            }
        }
    }

    bool PipelineBindingTable::updateParameter(const Name& paramName, TexturePtr tex, int element) {
        auto it = mName2BindingSlot.find(paramName);
        if (it != mName2BindingSlot.end()) {
            auto& pp = mBindingSlots[it->second];
            pp.varArr[element].set(tex);
            return true;
        }
        if (!RenderSystem::instance()->isBindlessEnabled())return false;

        auto bIt = mName2BindlessSlot.find(paramName);
        if (bIt != mName2BindlessSlot.end()) {
            auto& bItem = mBindlessItems[bIt->second];
            auto& type = bItem.type;
            if (type == UniformType::Count) {
                if (bItem.isUAV) {
                    type = UniformType::StorageImage;
                }
                else {
                    type = UniformType::Texture;
                }
            }

            if (type != UniformType::StorageImage && type != UniformType::Texture) {
                assert(false && "mismatch type");
                return false;
            }

            bItem.keepAliveRefs[element].set(tex);

            uint32_t globalIndex = EngineBindlessAPI::GetGlobalTextureIndex(&tex->getRsImage()->defaultView);

            if (bItem.bindlessData.size() <= element) {
                bItem.bindlessData.resize(bItem.location.bindlessInfo.count , INVALID_BINDLESS_INDEX);
            }

            if (bItem.bindlessData[element] != INVALID_BINDLESS_INDEX && bItem.bindlessData[element] != globalIndex) {
                EngineBindlessAPI::UnbindGlobalTexture(bItem.bindlessData[element]);
            }
            bItem.bindlessData[element] = globalIndex;

            auto fatherIt = mBindingPos2BindingSlot.find(bItem.location.bindingPos);
            if (fatherIt != mBindingPos2BindingSlot.end()) {
                auto& fatherUBO = mBindingSlots[fatherIt->second];
                void* uboData = nullptr;
                uint32_t size = 0;
                fatherUBO.varArr[0].getData(&uboData, &size);

                if (uboData) {
                    uint32_t stride = bItem.location.bindlessInfo.stride > 0 ? bItem.location.bindlessInfo.stride : sizeof(uint32_t);
                    uint32_t writeOffset = bItem.location.bindlessInfo.offset + (element * stride);

                    if (writeOffset + sizeof(uint32_t) <= size) {
                        std::memcpy(static_cast<uint8_t*>(uboData) + writeOffset, &globalIndex, sizeof(uint32_t));
                    }
                }
            }
            return true;
        }
        return false;
    }

    bool PipelineBindingTable::updateParameter(const Name& paramName, TexturePtr tex, ImageViewKey key, int element)
    {
        auto it = mName2BindingSlot.find(paramName);
        if (it != mName2BindingSlot.end()) {
            auto& pp = mBindingSlots[it->second];
            pp.varArr[element].set(tex);
            return true;
        }
        if (!RenderSystem::instance()->isBindlessEnabled())return false;

        auto bIt = mName2BindlessSlot.find(paramName);
        if (bIt != mName2BindlessSlot.end()) {
            auto& bItem = mBindlessItems[bIt->second];
            auto& type = bItem.type;
            if (type == UniformType::Count) {
                if (bItem.isUAV) {
                    type = UniformType::StorageImage;
                }
                else {
                    type = UniformType::Texture;
                }
            }

            if (type != UniformType::StorageImage && type != UniformType::Texture) {
                assert(false && "mismatch type");
                return false;
            }

            bItem.keepAliveRefs[element].set(tex,key);
            auto view = RenderSystem::instance()->getViewFromImage(tex->getRsImage(), key);
            uint32_t globalIndex = INVALID_BINDLESS_INDEX;
            if(type == UniformType::StorageImage){
                globalIndex = EngineBindlessAPI::GetGlobalRWTextureIndex(view);

            }else{
                globalIndex = EngineBindlessAPI::GetGlobalTextureIndex(view);
            }

            if (bItem.bindlessData.size() <= element) {
                bItem.bindlessData.resize(bItem.location.bindlessInfo.count, INVALID_BINDLESS_INDEX);
            }

            if (bItem.bindlessData[element] != INVALID_BINDLESS_INDEX && bItem.bindlessData[element] != globalIndex) {
                if (type == UniformType::StorageImage) {

                    EngineBindlessAPI::UnbindGlobalRWTexture(bItem.bindlessData[element]);
                }
                else {
                    EngineBindlessAPI::UnbindGlobalTexture(bItem.bindlessData[element]);
                }
                
            }
            bItem.bindlessData[element] = globalIndex;

            auto fatherIt = mBindingPos2BindingSlot.find(bItem.location.bindingPos);
            if (fatherIt != mBindingPos2BindingSlot.end()) {
                auto& fatherUBO = mBindingSlots[fatherIt->second];
                void* uboData = nullptr;
                uint32_t size = 0;
                fatherUBO.varArr[0].getData(&uboData, &size);

                if (uboData) {
                    uint32_t stride = bItem.location.bindlessInfo.stride > 0 ? bItem.location.bindlessInfo.stride : sizeof(uint32_t);
                    uint32_t writeOffset = bItem.location.bindlessInfo.offset + (element * stride);

                    if (writeOffset + sizeof(uint32_t) <= size) {
                        std::memcpy(static_cast<uint8_t*>(uboData) + writeOffset, &globalIndex, sizeof(uint32_t));
                    }
                }
            }
            return true;
        }
        return false;
    }

    bool PipelineBindingTable::updateParameter(const Name& paramName, rs_buffer* buffer, int element) {
        return updateParameter(paramName, buffer, 0, 0, element);
    }

    bool PipelineBindingTable::updateParameter(const Name& paramName, rs_buffer* buffer, uint32_t offset, uint32_t size, int element)
    {
        auto it = mName2BindingSlot.find(paramName);
        if (it != mName2BindingSlot.end()) {
            auto& pp = mBindingSlots[it->second];
            BufferPair bufferPair;
            bufferPair.buffer = buffer;
            bufferPair.offset = offset;
            bufferPair.size = size;

            pp.varArr[element].set(bufferPair);
            return true;
        }

        if (!RenderSystem::instance()->isBindlessEnabled())return false;

        auto bIt = mName2BindlessSlot.find(paramName);
        if (bIt != mName2BindlessSlot.end()) {

            auto& bItem = mBindlessItems[bIt->second];

            auto& type = bItem.type;

            if (type == UniformType::Count) {
                if (bItem.isUAV) {
                    type = UniformType::StorageBuffer;
                }
                else {
                    assert(false && "Uniform Buffer can not be bindless.");
                    type = UniformType::ConstantBuffer;
                }
            }

            if (type != UniformType::StorageBuffer&& type != UniformType::ConstantBuffer) {
                assert(false && "Different with previous binding");
                return false;
            }
            BufferPair bufferPair;
            bufferPair.buffer = buffer;
            bufferPair.offset = offset;
            bufferPair.size = size;
            bItem.keepAliveRefs[element].set(bufferPair);

            uint32_t dataSize = 0;
            uint64_t bindingData = EngineBindlessAPI::GetBlockBufferBindingData(buffer, dataSize);
            uint32_t slotStride = (dataSize == 8) ? 2 : 1;
            uint32_t dataIndex = element * slotStride;

            if (bItem.bindlessData.size() < (bItem.location.bindlessInfo.count * slotStride)) {
                bItem.bindlessData.resize(bItem.location.bindlessInfo.count * slotStride,INVALID_BINDING_POS);
            }

            uint64_t oldData = 0;
            if (slotStride == 2) {
                std::memcpy(&oldData, &bItem.bindlessData[dataIndex], 8);
            }
            else {
                oldData = bItem.bindlessData[dataIndex];
            }

            if (oldData != INVALID_BINDLESS_INDEX && oldData != bindingData) {
                EngineBindlessAPI::UnbindGlobalBuffer(oldData);
            }

            if (slotStride == 2) {
                std::memcpy(&bItem.bindlessData[dataIndex], &bindingData, 8);
            }
            else {
                bItem.bindlessData[dataIndex] = static_cast<uint32_t>(bindingData);
            }

            auto fatherIt = mBindingPos2BindingSlot.find(bItem.location.bindingPos);
            if (fatherIt != mBindingPos2BindingSlot.end()) {
                auto& fatherUBO = mBindingSlots[fatherIt->second];
                void* uboData = nullptr;
                uint32_t size = 0;
                fatherUBO.varArr[0].getData(&uboData, &size);

                if (uboData && dataSize > 0) {
                    uint32_t stride = bItem.location.bindlessInfo.stride > 0 ? bItem.location.bindlessInfo.stride : dataSize;
                    uint32_t writeOffset = bItem.location.bindlessInfo.offset + (element * stride);

                    if (writeOffset + dataSize <= size) {
                        std::memcpy(static_cast<uint8_t*>(uboData) + writeOffset, &bindingData, dataSize);
                    }
                }
            }
            return true;
        }
        return false;
    }

    bool PipelineBindingTable::updateParameter(const Name& paramName, SamplerPtr sampler, int element) {
        auto it = mName2BindingSlot.find(paramName);
        if (it != mName2BindingSlot.end()) {
            auto& pp = mBindingSlots[it->second];
            pp.varArr[element].set(sampler);
            return true;
        }
        if (!RenderSystem::instance()->isBindlessEnabled())return false;

        auto bIt = mName2BindlessSlot.find(paramName);

        if (bIt != mName2BindlessSlot.end()) {
            auto& bItem = mBindlessItems[bIt->second];

            auto& type = bItem.type;

            if (type == UniformType::Count) {
                if (bItem.isUAV != false) {
                    assert(false && "MUST BE SRV");
                    return false;
                }
                type = UniformType::Sampler;
            }

            if (type != UniformType::Sampler) {
                assert(false && "mismatch type");
                return false;
            }

            bItem.keepAliveRefs[element].set(sampler);

            uint32_t globalIndex = EngineBindlessAPI::GetGlobalSamplerIndex(sampler->getRsSampler());

            if (bItem.bindlessData.size() <= element) {
                bItem.bindlessData.resize(bItem.location.bindlessInfo.count, INVALID_BINDLESS_INDEX);
            }

            if (bItem.bindlessData[element] != INVALID_BINDLESS_INDEX && bItem.bindlessData[element] != globalIndex) {
                EngineBindlessAPI::UnbindGlobalSampler(bItem.bindlessData[element]);
            }
            bItem.bindlessData[element] = globalIndex;

            auto fatherIt = mBindingPos2BindingSlot.find(bItem.location.bindingPos);
            if (fatherIt != mBindingPos2BindingSlot.end()) {
                auto& fatherUBO = mBindingSlots[fatherIt->second];
                void* uboData = nullptr;
                uint32_t size = 0;
                fatherUBO.varArr[0].getData(&uboData, &size);

                if (uboData) {
                    uint32_t stride = bItem.location.bindlessInfo.stride > 0 ? bItem.location.bindlessInfo.stride : sizeof(uint32_t);
                    uint32_t writeOffset = bItem.location.bindlessInfo.offset + (element * stride);

                    if (writeOffset + sizeof(uint32_t) <= size) {
                        std::memcpy(static_cast<uint8_t*>(uboData) + writeOffset, &globalIndex, sizeof(uint32_t));
                    }
                }
            }
            return true;
        }
        return false;
    }

    bool PipelineBindingTable::updateParameterData(const Name& paramName, const void* data, uint32_t dataSize, int element) {
        auto it = mName2BindingSlot.find(paramName);
        if (it != mName2BindingSlot.end()) {
            auto& pp = mBindingSlots[it->second];

            if (pp.location.descriptorInfo.type == UniformType::UniformBuffer) {
                void* destData = nullptr;
                uint32_t bufferSize = 0;
                pp.varArr[element].getData(&destData, &bufferSize);

                if (destData && dataSize <= bufferSize) {
                    std::memcpy(destData, data, dataSize);
                    return true;
                }
            }
        }
        return false;
    }

} // namespace Render