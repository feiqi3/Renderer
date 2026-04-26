#ifndef COMPUTE_KERNEL_H
#define COMPUTE_KERNEL_H

#include "render_resource.h"
#include "render_resource_createinfo.h"
#include "common/NoCopyable.h"
#include "Renderer/ResourceVariant.h"
#include "Renderer/PipelineBindingTable.h"
#include <string>
#include <vector>
#include <map>
#include <optional>

namespace Render {

    struct rs_compute_pipeline;
    struct rs_shader_module;
    struct rs_drawdata;
    struct rs_pipeline;
    struct rs_commandbuffer;

    class ComputeKernel : public Common::NonCopyable {
    public:
        ComputeKernel(const std::string& shaderPath, const MacroPairs& macros);
        ~ComputeKernel();

        inline bool isValid() const { return mPipeline != nullptr; }

        void setParameter(const std::string& name, rs_buffer* buffer,int element = 0);
        void setParameter(const std::string& name, rs_buffer* buffer , uint32_t offset, uint32_t size, int element = 0);
        void setParameter(const std::string& name, TexturePtr texture, int element = 0);
        void setParameter(const std::string& name, TexturePtr texture,ImageViewKey key, int element = 0);
        void setParameter(const std::string& name, SamplerPtr sampler, int element = 0);
        void setParameter(const std::string& name, const void* data, uint32_t size);

        template<typename T>
        void setParameter(const std::string& name, const T& data) {
            setParameter(name, &data, sizeof(T));
        }

        void dispatch(rs_commandbuffer* cmd, uint32_t groupX, uint32_t groupY, uint32_t groupZ);

    private:
        rs_compute_pipeline* mPipeline = nullptr;
        rs_drawdata* mCurrentDrawData = nullptr;

        PipelineBindingTable    mBindingTable;

        std::map<std::string, RenderResourceVariant> mPendingParams;

        
    };

}

#endif