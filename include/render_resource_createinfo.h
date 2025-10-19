#ifndef RENDER_RESOURCE_CREATE_INFO
#define RENDER_RESOURCE_CREATE_INFO
#include "render_resource_def.h"
#include <string>
#include <Vector>
#include <functional>
#include "limits.h"
namespace Render {


    struct BackEndInitDesc {
        std::string appName;
        std::string engineName;
        bool enableValidation = true;
        bool asyncTransferCompute = false;
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
        const char* shaderCode;
        const char* entryPoint = "main";
        size_t codeSizeByte = 0;
        ShaderStage   stage;       // 阶段
        bool isSpirv = false;
        struct ShaderCompileDesc* compileDesc = 0;
    };

    struct CommandBufferDesc {
        QueueType queueType;
        bool transient = false; //Will buffers be used multiTimes in oneframe?
        bool isSecondary = false;
    };

    enum class StorageOp : uint8_t {
        Cached,   
        Clear,      
        DontCare  
    };

    struct PassAttachment {
        StorageOp      loadOp;
        StorageOp     storeOp;
        bool isHDR = false; //float rt is HDR
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
        
        Topology topology = Topology::TriangleList;
        FillMode fillMode = FillMode::Fill;
        CullMode cullMode = CullMode::None;
        FrontFace frontFace = FrontFace::ClockWise;

        // 多重采样
        uint32_t    rasterizationSamples = 1;
    };

    //Binding Position is the declared sequence in pipeline buffer list
    struct InputBufferBinding {
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
    struct rs_shader_module;
    struct PipelineDesc {
        PipelineType         type;         // Graphics / Compute / RayTracing
        std::vector<rs_shader_module*> shaders;
        RenderState          renderState;  // 仅 Graphics 有效
        VertexInputDescription vertexInputDesc;
    };

    using rs_binding_pos = uint32_t;
#define INVALID_BINDING_POS UINT_MAX
    struct BindingInfo {
        std::string bindingItemName;
        rs_binding_pos bindingPos; //Platform related
        uint16_t shaderVisibleStage = 0; //shader stage
        uint16_t count = 0; 
        uint16_t size = 0; 
        ResourceType type;
    };

    struct Rect2D {
        float l, r, t, b;
    };

    struct VertexBindingInfo {
        uint16_t offset;
        struct rs_buffer* buffer;
    };

    struct RenderInfo {
        std::vector<VertexBindingInfo > bindingBuffers;                      //in buffers
        struct rs_buffer* indexBuffer = 0;
        IndexType indexType = IndexType::Uint32;
        uint32_t idxOffset = 0;
        uint32_t idxCount = 0;
        uint32_t vtxoffset = 0;
        uint32_t instanceCount = 1;
        bool isIndirect = false;
    };

    struct ShaderIncludeRes {
        std::string ShaderName;
        std::string ShaderContent;
    };

    using ShaderIncFindFunc = std::function<ShaderIncludeRes(const std::vector<std::string>&, const std::string&)>;
    
    struct ShaderCompileDesc {
        ShaderStage stage;
        ShaderLang langType = ShaderLang::GLSL;
        std::string shaderName;
        std::string shaderSrcCode;
        bool enableOptimize = false;
        bool generateDebugInfo = false;
        std::vector<std::string> shaderIncludeDirectories;
        ShaderIncFindFunc shaderIncludeFindFunc = nullptr;
    };

};

#endif