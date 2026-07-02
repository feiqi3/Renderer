#pragma once

#include "render_resource.h"
#include "common/Name.h"
#include "common/SmallVector.h"
#include "Renderer/ResourceVariant.h"
#include "Renderer/Texture.h"
#include "Renderer/SamplerResourceManager.h"
#include <vector>
#include <map>

namespace Render {

    struct rs_pipeline;

    class PipelineBindingTable {
    public:
        PipelineBindingTable() = default;
        ~PipelineBindingTable();

		bool commit(rs_drawdata* drawdata, bool allowMultiCommit = false);
        bool commit(rs_pipeline* pipeline,rs_drawdata* drawdata, bool allowMultiCommit = false);
        void init(const Name& passName, rs_pipeline* pipeline);

        bool updateParameter(const Name& paramName, TexturePtr tex, int element = 0);
		bool updateParameter(const Name& paramName, TexturePtr tex, rs_image_view* view, int element = 0);
		bool updateParameter(const Name& paramName, TexturePtr tex, ImageViewKey key, int element = 0);
        bool updateParameter(const Name& paramName, rs_buffer* buffer, int element = 0);
        bool updateParameter(const Name& paramName, rs_buffer* buffer, uint32_t offset, uint32_t size, int element = 0);
        bool updateParameter(const Name& paramName, SamplerPtr sampler, int element = 0);
        bool updateParameterData(const Name& paramName, const void* data, uint32_t size, int element = 0);

        const Name& getPassName() const { return mPassName; }
        rs_pipeline* getPipeline() const { return mPipeline; }

		struct _BindlessItem {
			UniformType type = UniformType::Count;
			ResourceLocation location;
			SmallVector<RenderResourceVariant, 1> keepAliveRefs;
			SmallVector<uint64_t, 1> bindlessData;				//64 bit for BDA index or global index
		};

		struct _ParameterPair {
			ResourceLocation location;
			ImageType parameterImageType = ImageType::Invalid;
			SmallVector<RenderResourceVariant, 1> varArr;
		};

    private:


        Name mPassName;
        rs_pipeline* mPipeline = nullptr;

        std::vector<_ParameterPair> mBindingSlots;
        std::vector<_BindlessItem> mBindlessItems;

        std::map<Name, uint32_t> mName2BindingSlot;
        std::map<Name, uint32_t> mName2BindlessSlot;
        std::map<rs_binding_pos, uint32_t> mBindingPos2BindingSlot;
	};
	class MaterialBindingTable {
	public:
		MaterialBindingTable() = default;
		~MaterialBindingTable() = default;

		inline void unregisterPipeline(const Name& passName, rs_pipeline* pipeline) {
			if (mTables.find(passName) != mTables.end()) {
				mTables.erase(passName);
				mPipelineToName.erase(pipeline);
			}
		}

		inline void registerPipeline(const Name& passName, rs_pipeline* pipeline) {
			if (mTables.find(passName) == mTables.end()) {
				mTables[passName].init(passName, pipeline);
				mPipelineToName[pipeline] = passName;

				auto& newTable = mTables[passName];
				for (const auto& [paramName, elementMap] : mParameterCache) {
					for (const auto& [element, variant] : elementMap) {

						if (variant.isTexture()) {
							newTable.updateParameter(paramName, variant.getTexture(), element);
						}
						else if (variant.isTextureView()) {
							auto viewPair = variant.getTextureView();
							newTable.updateParameter(paramName, viewPair->tex, viewPair->view, element); 
						}
						else if (variant.isSampler()) {
							newTable.updateParameter(paramName, variant.getSampler(), element);
						}
						else if (variant.isBuffer()) {
							auto bufferPair = variant.getBufferPair();
							newTable.updateParameter(paramName, bufferPair->buffer, bufferPair->offset, bufferPair->size, element);
						}
						else if (variant.isUniformBuffer()) {
							void* data = nullptr;
							uint32_t size = 0;
							variant.getData(&data, &size);
							if (data && size > 0) {
								newTable.updateParameterData(paramName, data, size, element);
							}
						}
					}
				}
			}
		}

		template<typename T>
		inline bool updateParameter(const Name& passName, const Name& paramName, const T& parameter, int element = 0) {
			auto it = mTables.find(passName);
			if (it != mTables.end()) {
				return it->second.updateParameter(paramName, parameter, element);
			}
			return false;
		}

		template<typename T>
		inline bool updateParameter(rs_pipeline* pipeline, const Name& paramName, const T& parameter, int element = 0) {
			auto nameIt = mPipelineToName.find(pipeline);
			if (nameIt != mPipelineToName.end()) {
				return updateParameter(nameIt->second, paramName, parameter, element);
			}
			return false;
		}

		inline void broadcastParameter(const Name& paramName, const TexturePtr& parameter, int element = 0) {
			mParameterCache[paramName][element].set(parameter);
			for (auto& pair : mTables) {
				pair.second.updateParameter(paramName, parameter, element);
			}
		}

		inline void broadcastParameter(const Name& paramName, const TexturePtr& parameter, ImageViewKey key, int element = 0) {
			mParameterCache[paramName][element].set(parameter, key);
			for (auto& pair : mTables) {
				pair.second.updateParameter(paramName, parameter, key, element);
			}
		}

		inline void broadcastParameter(const Name& paramName, rs_buffer* parameter, int element = 0) {
			BufferPair bp;
			bp.buffer = parameter;
			bp.offset = 0;
			bp.size = 0;
			mParameterCache[paramName][element].set(bp);

			for (auto& pair : mTables) {
				pair.second.updateParameter(paramName, parameter, element);
			}
		}

		inline void broadcastParameter(const Name& paramName, const SamplerPtr& parameter, int element = 0) {
			mParameterCache[paramName][element].set(parameter);
			for (auto& pair : mTables) {
				pair.second.updateParameter(paramName, parameter, element);
			}
		}

		inline void broadcastParameterData(const Name& paramName, const void* data, uint32_t size, int element = 0) {
			mParameterCache[paramName][element].setUniformBuffer(data, size);
			for (auto& pair : mTables) {
				pair.second.updateParameterData(paramName, data, size, element);
			}
		}

		PipelineBindingTable* getPipelineBindingTable(const Name& name);
		PipelineBindingTable* getPipelineBindingTable(rs_pipeline* pipeline);

	private:
		std::map<Name, PipelineBindingTable> mTables;
		std::map<rs_pipeline*, Name> mPipelineToName;

		std::map<Name, std::map<int, RenderResourceVariant>> mParameterCache;
	};

} // namespace Render