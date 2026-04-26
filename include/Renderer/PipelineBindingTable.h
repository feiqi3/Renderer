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

        void commit(rs_pipeline* pipeline,rs_drawdata* drawdata);

        void init(const Name& passName, rs_pipeline* pipeline);

        bool updateParameter(const Name& paramName, TexturePtr tex, int element = 0);
        bool updateParameter(const Name& paramName, TexturePtr tex, ImageViewKey key, int element = 0);
        bool updateParameter(const Name& paramName, rs_buffer* buffer, int element = 0);
        bool updateParameter(const Name& paramName, rs_buffer* buffer, uint32_t offset, uint32_t size, int element = 0);
        bool updateParameter(const Name& paramName, SamplerPtr sampler, int element = 0);
        bool updateParameterData(const Name& paramName, const void* data, uint32_t size, int element = 0);

        const Name& getPassName() const { return mPassName; }
        rs_pipeline* getPipeline() const { return mPipeline; }

    private:
        struct _BindlessItem {
            bool isUAV = false;
            UniformType type = UniformType::Count;
            ResourceLocation location;
            SmallVector<RenderResourceVariant, 1> keepAliveRefs;
            SmallVector<uint32_t, 2> bindlessData;
        };

        struct _ParameterPair {
            ResourceLocation location;
            ImageType parameterImageType = ImageType::Invalid;
            SmallVector<RenderResourceVariant, 1> varArr;
        };

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

        template<typename T>
        inline void broadcastParameter(const Name& paramName, const T& parameter, int element = 0) {
            for (auto& pair : mTables) {
                pair.second.updateParameter(paramName, parameter, element);
            }
        }

        inline void broadcastParameter(const Name& paramName, const TexturePtr& parameter, ImageViewKey key, int element = 0) {
            for (auto& pair : mTables) {
                pair.second.updateParameter(paramName, parameter, key, element);
            }
        }

        inline void broadcastParameterData(const Name& paramName, const void* data, uint32_t size, int element = 0) {
            for (auto& pair : mTables) {
                pair.second.updateParameterData(paramName, data, size, element);
            }
        }

        PipelineBindingTable* getPipelineBindingTable(const Name& name);
        PipelineBindingTable* getPipelineBindingTable(rs_pipeline* pipeline);

    private:
        std::map<Name, PipelineBindingTable> mTables;
        std::map<rs_pipeline*, Name> mPipelineToName;
    };

} // namespace Render