#include "Renderer/PipelineBindingTable.h"
#include "Renderer/RenderSystem.h"
#include <cstring> 

namespace Render {
    namespace EngineBindlessAPI {
        static uint32_t GetGlobalSamplerIndex(rs_sampler* sampler) {
            return RenderSystem::instance()->updateGlobalBindlessDataSampler(RenderSystem::instance()->getGlobalBindlessData(), sampler);
        }
        static uint32_t UnbindGlobalSampler(uint32_t index) {
            return RenderSystem::instance()->unbindGlobalBindlessDataSampler(RenderSystem::instance()->getGlobalBindlessData(), index);
        }

        static uint32_t GetGlobalTextureIndex(rs_image_view* img) {
            return RenderSystem::instance()->updateGlobalBindlessDataTexture(RenderSystem::instance()->getGlobalBindlessData(), img);
        }
        static uint32_t GetGlobalRWTextureIndex(rs_image_view* img) {
            return RenderSystem::instance()->updateGlobalBindlessDataRWTexture(RenderSystem::instance()->getGlobalBindlessData(), img);
        }

        static uint32_t UnbindGlobalTexture(uint32_t index) {
            return RenderSystem::instance()->unbindGlobalBindlessDataTexture(RenderSystem::instance()->getGlobalBindlessData(), index);
        }
        static uint32_t UnbindGlobalRWTexture(uint32_t index) {

            return RenderSystem::instance()->unbindGlobalBindlessDataRWTexture(RenderSystem::instance()->getGlobalBindlessData(), index);
        }
        // 8 (BDA)
        static uint64_t GetBlockBufferBindingData(rs_buffer* buffer,uint32_t offset) {
            // BDA --> we get device address and offset it
            auto binding = RenderSystem::instance()->updateGlobalBindlessDataBuffer(RenderSystem::instance()->getGlobalBindlessData(), buffer);
            return binding + offset;
        }
        static uint64_t UnbindGlobalBuffer(uint64_t data) {
            return RenderSystem::instance()->unbindGlobalBindlessDataBuffer(RenderSystem::instance()->getGlobalBindlessData(), data);

        }

        static void markResourceUAV(rs_buffer* buffer) {
            RenderSystem::instance()->markGlobalBindlessDataBuffer(RenderSystem::instance()->getGlobalBindlessData(), buffer);
        }

        static void markResourceUAV(rs_image_view* view) {
			RenderSystem::instance()->markGlobalBindlessDataRWTexture(RenderSystem::instance()->getGlobalBindlessData(), view);
        }

        static void markResourceSRV(rs_buffer* buffer) {
			RenderSystem::instance()->markGlobalBindlessDataRWBuffer(RenderSystem::instance()->getGlobalBindlessData(), buffer);
		}

        static void markResourceSRV(rs_image_view* view) {
			RenderSystem::instance()->markGlobalBindlessDataTexture(RenderSystem::instance()->getGlobalBindlessData(), view);
		}

        static inline void initFatherUBO(PipelineBindingTable::_ParameterPair& pair) {
            if (pair.varArr[0].isValid()) {
                return;
            }

            if (pair.location.descriptorInfo.type != UniformType::UniformBuffer)return;

            pair.varArr[0].setUniformBuffer(nullptr, pair.location.descriptorInfo.size);
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
                    EngineBindlessAPI::GetBlockBufferBindingData(nullptr, 0);
                    //Get old binding pos.
                    if (i < bItem.bindlessData.size()) {
                        uint64_t oldData = 0;
                        oldData = bItem.bindlessData[i];
                        if (oldData != 0XFFFFFFFFFFFFFFFF) EngineBindlessAPI::UnbindGlobalBuffer(oldData);
                    }
                }
                else if (var.getTexture()) {
                    if (i < bItem.bindlessData.size()) {
                        uint32_t oldData = bItem.bindlessData[i];
                        if (oldData != INVALID_BINDLESS_INDEX) EngineBindlessAPI::UnbindGlobalTexture(oldData);
                    }
                }
                else if (var.getSampler()) {
                    if (i < bItem.bindlessData.size()) {
                        uint32_t oldData = bItem.bindlessData[i];
                        if (oldData != INVALID_BINDLESS_INDEX) EngineBindlessAPI::UnbindGlobalSampler(oldData);
                    }
                }
            }
        }
    }

    bool PipelineBindingTable::commit(rs_pipeline* pipeline, rs_drawdata* drawdata,bool allowMultiCommit)
    {
        auto sys = RenderSystem::instance()->instance();


        if (!allowMultiCommit && drawdata->lastCommitFrame == sys->getNextRenderFrame()) {
            //avoid redundant uniform update.
            return false;
        }

        drawdata->lastCommitFrame = sys->getNextRenderFrame();


        for (auto& pp : mBindingSlots) {

            for (int i = 0; i < pp.varArr.size(); ++i) {
                auto& var = pp.varArr[i];
                if (!var.isValid()) {
                    continue;
                }
                switch (pp.location.descriptorInfo.type) {
                case UniformType::StorageBuffer:
                case UniformType::ConstantBuffer:
                {
                    auto bufferPair = var.getBufferPair();
                    if (!bufferPair->buffer) {
                        break;
                    }
                    sys->updateUniform(pp.location.bindingPos, i, bufferPair->buffer, pipeline, drawdata, bufferPair->offset, bufferPair->size);
                    break;
                }
                case UniformType::UniformBuffer:
                {
                    //TODO: ubo idx
                    void* dataptr = nullptr;
                    uint32_t size = 0;
                    auto& bufferPtr = *(var.getUniformDataPtr());
                    dataptr         = bufferPtr.get();
                    size            = bufferPtr.size();
                    if (dataptr && size > 0) {
                        bufferPtr.setDirty(false);
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
                        if (!viewPair->view)break;
                        sys->updateUniform(pp.location.bindingPos, i, viewPair->view, pipeline, drawdata);
                    }else if(var.isTexture()) {
                        if (!var.getTexture())break;
                        sys->updateUniform(pp.location.bindingPos, i, var.getTexture()->getRsImage(), pipeline, drawdata);
                    }
                    break;
                }

                case UniformType::Sampler:
                    if (!var.getSampler())break;
                    sys->updateUniform(pp.location.bindingPos, i, var.getSampler()->getRsSampler(), pipeline, drawdata);
                    break;

                default:
                    break;
                }
            }

        }
        if (!RenderSystem::instance()->isBindlessEnabled())return true;

        for (const auto& pp : mBindlessItems) {
            switch (pp.type)
            {
            case UniformType::ConstantBuffer:
            case UniformType::UniformBuffer:
            {
                for (int i = 0;i < pp.bindlessData.size();++i) {
					if (!pp.keepAliveRefs[i].isValid())continue;
					const auto& data = pp.keepAliveRefs[i].getBufferPair();
                    if (!data)continue;
                    EngineBindlessAPI::markResourceSRV(data->buffer);
                }
                break;
            }
            case UniformType::StorageBuffer:
            {
				for (int i = 0;i < pp.bindlessData.size();++i) {
					if (!pp.keepAliveRefs[i].isValid())continue;
					const auto& data = pp.keepAliveRefs[i].getBufferPair();
					if (!data)continue;
					EngineBindlessAPI::markResourceUAV(data->buffer);
				}
				break;
            }
			case UniformType::StorageImage:
			{
				for (int i = 0;i < pp.bindlessData.size();++i) {
					if (!pp.keepAliveRefs[i].isValid())continue;
					rs_image_view* view = nullptr;
                    const auto& refVar = pp.keepAliveRefs[i];
                    if (refVar.getTexture()) {
                        view = refVar.getTexture()->getRsImage()->defaultView;
                    }
                    else if (
                        refVar.getTextureView()
                        ) {
                        view = refVar.getTextureView()->view;
                    }
                    if (!view)continue;
					EngineBindlessAPI::markResourceUAV(view);
				}
				break;
			}
			case UniformType::Texture:
			{
				for (int i = 0;i < pp.bindlessData.size();++i) {
                    if (!pp.keepAliveRefs[i].isValid())continue;
                    rs_image_view* view = nullptr;
					const auto& refVar = pp.keepAliveRefs[i];
					if (refVar.isTexture()) {
						view = refVar.getTexture()->getRsImage()->defaultView;
					}
					else if (
						refVar.isTextureView()
						) {
                        view = refVar.getTextureView()->view;
					}
					EngineBindlessAPI::markResourceSRV(view);
				}
				break;
			}
            default:
                break;
            }

        }

        return true;
    }

	bool PipelineBindingTable::commit(rs_drawdata* drawdata, bool allowMultiCommit /*= false*/)
	{
        return commit(mPipeline, drawdata, allowMultiCommit);
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

            if (RenderSystem::instance()->isEngineResourceName(loc.itemName)) {
                //Do dot save any bindless/scene/object/camera related binding slots.
                continue;
            }

            if (loc.type == ResourceLocationType::BindingSlot) {
                _ParameterPair pp;
                pp.location = loc;
                pp.parameterImageType = ImageType::Invalid;
                pp.varArr.resize(loc.descriptorInfo.count);

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
                bindlessItem.type = loc.bindlessInfo.type;
                uint32_t realUsedSlots = loc.bindlessInfo.count;
                bindlessItem.bindlessData.resize(loc.bindlessInfo.count, INVALID_BINDLESS_INDEX);
                mBindlessItems.push_back(std::move(bindlessItem));
                mName2BindlessSlot[loc.itemName] = static_cast<uint32_t>(mBindlessItems.size() - 1);
            }
        }
    }

    bool PipelineBindingTable::updateParameter(const Name& paramName, TexturePtr tex, int element) {
        auto view = tex ? tex->getRsImage()->defaultView : nullptr;
        return updateParameter(paramName, tex, view, element);
    }

    bool PipelineBindingTable::updateParameter(const Name& paramName, TexturePtr tex, ImageViewKey key, int element)
    {
        rs_image_view* view = nullptr;
        if (tex != nullptr) {
            view = RenderSystem::instance()->getViewFromImage(tex->getRsImage(), key);
        }
        else {
            return updateParameter(paramName, tex);
        }
        return updateParameter(paramName, tex, RenderSystem::instance()->getViewFromImage(tex->getRsImage(),key));
    }

	bool PipelineBindingTable::updateParameter(const Name& paramName, TexturePtr tex, rs_image_view* view, int element /*= 0*/)
	{
		auto it = mName2BindingSlot.find(paramName);
		if (it != mName2BindingSlot.end()) {
			auto& pp = mBindingSlots[it->second];
            pp.varArr[element].set(tex,view);
			return true;
		}
		if (!RenderSystem::instance()->isBindlessEnabled())return false;

		auto bIt = mName2BindlessSlot.find(paramName);
		if (bIt != mName2BindlessSlot.end()) {
			auto& bItem = mBindlessItems[bIt->second];
			auto& type = bItem.type;

            if (view && view->viewKey.getUAVAccess() == UAVAccess::ReadOnly && type == UniformType::StorageImage) {
                assert(false && "Cannot bind a srv to uav");
            }

            if (view && view->viewKey.getUAVAccess() != UAVAccess::ReadOnly && type == UniformType::Texture) {
				assert(false && "Cannot bind a uav to srv");
            }

			if (type != UniformType::StorageImage && type != UniformType::Texture) {
				assert(false && "mismatch type");
				return false;
			}

			uint32_t globalIndex = INVALID_BINDLESS_INDEX;
			if (type == UniformType::StorageImage) {
				globalIndex = EngineBindlessAPI::GetGlobalRWTextureIndex(view);

			}
			else {
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

            if (tex && view) {
                bItem.keepAliveRefs[element].set(tex, view);
                bItem.bindlessData[element] = globalIndex;
            }

			auto fatherIt = mBindingPos2BindingSlot.find(bItem.location.bindingPos);
			if (fatherIt != mBindingPos2BindingSlot.end()) {
				auto& fatherUBO = mBindingSlots[fatherIt->second];
				void* uboData = nullptr;
				uint32_t size = 0;
                EngineBindlessAPI::initFatherUBO(fatherUBO);

				uint32_t stride = bItem.location.bindlessInfo.stride;
				uint32_t writeOffset = bItem.location.bindlessInfo.offset + (element * stride);
				fatherUBO.varArr[0].writeData(&globalIndex, sizeof(uint32_t), writeOffset);
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

            if (type != UniformType::StorageBuffer&& type != UniformType::ConstantBuffer) {
                assert(false && "Different with previous binding");
                return false;
            }
            BufferPair bufferPair;
            bufferPair.buffer = buffer;
            bufferPair.offset = offset;
            bufferPair.size = size;
            bItem.keepAliveRefs[element].set(bufferPair);

            uint64_t bindingData = EngineBindlessAPI::GetBlockBufferBindingData(buffer, offset);
            uint32_t dataIndex = element;

            if (bItem.bindlessData.size() < (bItem.location.bindlessInfo.count)) {
                bItem.bindlessData.resize(bItem.location.bindlessInfo.count,0XFFFFFFFFFFFFFFFF);
            }

            uint64_t oldData = 0;
            oldData = bItem.bindlessData[dataIndex];
            //In bindless mode or BDA mode
            if ( (oldData != INVALID_BINDLESS_INDEX && oldData!= 0xFFFFFFFFFFFFFFFF ) && oldData != bindingData) {
                EngineBindlessAPI::UnbindGlobalBuffer(oldData);
            }

            bItem.bindlessData[dataIndex] = static_cast<uint32_t>(bindingData);

            auto fatherIt = mBindingPos2BindingSlot.find(bItem.location.bindingPos);
            if (fatherIt != mBindingPos2BindingSlot.end()) {
                auto& fatherUBO = mBindingSlots[fatherIt->second];
                void* uboData = nullptr;
                uint32_t size = 0;
                EngineBindlessAPI::initFatherUBO(fatherUBO);

				uint32_t stride = bItem.location.bindlessInfo.stride;
				uint32_t writeOffset = bItem.location.bindlessInfo.offset + (element * stride);

                fatherUBO.varArr[0].writeData(&bindingData, sizeof(uint64_t), writeOffset);
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

            if (type != UniformType::Sampler) {
                assert(false && "mismatch type");
                return false;
            }

            bItem.keepAliveRefs[element].set(sampler);

            uint32_t globalIndex = EngineBindlessAPI::GetGlobalSamplerIndex(sampler ? sampler->getRsSampler(): nullptr);

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
                EngineBindlessAPI::initFatherUBO(fatherUBO);

				uint32_t stride = bItem.location.bindlessInfo.stride;
				uint32_t writeOffset = bItem.location.bindlessInfo.offset + (element * stride);
				fatherUBO.varArr[0].writeData(&globalIndex, sizeof(uint32_t), writeOffset);

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
                if (!pp.varArr[element].isValid()) {
                    pp.varArr[element].setUniformBuffer(nullptr, pp.location.descriptorInfo.size);
                }

                pp.varArr[element].writeData(data, dataSize,0);
            }
        }
        return false;
    }

} // namespace Render