#ifndef RENDER_RESOURCE_CREATE_INFO
#define RENDER_RESOURCE_CREATE_INFO
#include "render_resource_def.h"
#include <Vector>
namespace Render {


    struct BackEndInitDesc {
        FrontFace frontFaceSet = FrontFace::ClockWise;
    };

    struct BufferDesc {
        size_t byteSize;
        uint32_t bufUsage;
        uint8_t queueType;
        bool mappable = false;
    };

    struct ImageDesc {
        uint16_t        width;
        uint16_t        height;
        uint16_t        depth = 1;
        uint16_t        mipLevels = 1;
        uint16_t        arrayLayers = 1;
        ImageType       type = ImageType::V2D;
        ImageFormat     format;
        uint32_t        usage;
        SampleCount samples = SampleCount::Count1;
    };

    struct SamplerDesc {
        AddressMode     addressU = AddressMode::Repeat;
        AddressMode     addressV = AddressMode::Repeat;
        AddressMode     addressW = AddressMode::Repeat;
        Filter          minFilter = Filter::Linear;
        Filter          magFilter = Filter::Linear;
        MipMapMode      mipmapMode = MipMapMode::Linear;
        bool            enableAnisotropy = false;
        float           maxAnisotropy = 1.0f;
        CompareOp       compareOp = CompareOp::Never;   // 用于 Shadow samplers
        bool            unnormalizedCoords = false;
        BorderColor     borderColor = BorderColor::FloatOpaqueBlack;
        bool            enableCompare = false;
    };

    struct ShaderDesc {
        char* shaderCode;
        const char* entryPoint = "main";
        size_t codeSizeByte = 0;
        ShaderStage   stage;       // 阶段
    };

    struct CommandBufferDesc {
        QueueType queueType;
        bool transient = true; //Will buffers be used multiTimes?
        bool isSecondary = false;
    };

    enum class StorageOp : uint8_t {
        Cached,   
        Clear,      
        DontCare  
    };

    struct PassAttachment {
        ImageFormat format;
        ImageUsage  usage;
        SampleCount samples = SampleCount::Count1;
        StorageOp      loadOp;
        StorageOp     storeOp;
    };

    struct PassDesc {
        std::vector<PassAttachment> attachments;
        bool writeDepth = true;
    };

    struct BlendState {
        bool        blendEnable = false;
        BlendFactor srcColorBlend = BlendFactor::One;
        BlendFactor dstColorBlend = BlendFactor::Zero;
        BlendOp     colorBlendOp = BlendOp::Add;
        BlendFactor srcAlphaBlend = BlendFactor::One;
        BlendFactor dstAlphaBlend = BlendFactor::Zero;
        BlendOp     alphaBlendOp = BlendOp::Add;
    };

    struct RenderState {
        // 光栅化
        bool        depthTestEnable = true;
        bool        depthWriteEnable = true;
        CompareOp   depthCompareOp = CompareOp::Less;
        bool        stencilTestEnable = false;

        // 模板测试
        bool        stencilTestEnable = false;                  // 是否启用模板测试
        CompareOp   stencilCompareOp = CompareOp::Always;      // 模板比较操作
        StencilOp   stencilFailOp = StencilOp::Keep;        // 模板比较失败后的操作
        StencilOp   stencilPassOp = StencilOp::Keep;        // 模板比较与深度测试都通过后的操作
        StencilOp   stencilDepthFailOp = StencilOp::Keep;        // 模板比较通过但深度测试失败后的操作
        uint32_t    stencilReadMask = 0xFF;                   // 模板比较掩码
        uint32_t    stencilWriteMask = 0xFF;                   // 模板写入掩码
        uint32_t    stencilReference = 0;                      // 模板参考值

        std::vector<BlendState> blendStates;
        
        Topology topology;
        FillMode fillMode;
        CullMode cullMode;
        // 多重采样
        uint32_t    rasterizationSamples = 1;
    };

    struct InputBufferBinding {
        uint32_t    binding;       // vertex buffer binding index
        uint32_t    stride;        // bytes per vertex / instance
        bool        perInstance;   // false = per-vertex, true = per-instance
    };

    struct InputAttribute {
        uint32_t        location;      // shader location
        uint32_t        binding;       // binding index
        VertexFormat    format;
        uint32_t        offset;        // byte offset
    };

    struct VertexInputDescription {
        std::vector<InputBufferBinding>   bindings;
        std::vector<InputAttribute> attributes;
    };

    struct PipelineDesc {
        PipelineType         type;         // Graphics / Compute / RayTracing
        std::vector<rs_shader_module*> shaders;
        RenderState          renderState;  // 仅 Graphics 有效
        VertexInputDescription vertexInputDesc;
    };

    struct BindingInfo {
        uint16_t shaderVisibleStage = 0; //shader stage
        uint16_t count = 0; 
        uint16_t size = 0; 
        uint8_t binding = 0;
        ResourceType type;
    };

};

#endif