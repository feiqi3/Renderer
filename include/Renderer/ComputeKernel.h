#ifndef COMPUTE_KERNEL_H
#define COMPUTE_KERNEL_H

#include "render_resource.h"
#include "render_resource_createinfo.h"
#include "common/NoCopyable.h"
#include "Renderer/ResourceVariant.h"
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
        ComputeKernel(const std::string& shaderPath, const std::string& marco = "");
        ~ComputeKernel();

        inline bool isValid() const { return mPipeline != nullptr; }

        void setParameter(const std::string& name, rs_buffer* buffer);
        void setParameter(const std::string& name, TexturePtr texture);
        void setParameter(const std::string& name, SamplerPtr sampler);
        void setParameter(const std::string& name, const void* data, uint32_t size);

        template<typename T>
        void setParameter(const std::string& name, const T& data) {
            setParameter(name, &data, sizeof(T));
        }

        void dispatch(rs_commandbuffer* cmd, uint32_t groupX, uint32_t groupY, uint32_t groupZ);

    private:
        std::optional<BindingInfo> getBindingInfoByName(const std::string& name) const;

    private:
        rs_compute_pipeline* mPipeline = nullptr;
        std::map<std::string, BindingInfo> mBindingTable;

        rs_drawdata* mCurrentDrawData = nullptr;

        std::map<std::string, RenderResourceVariant> mPendingParams;
    };

}

#endif