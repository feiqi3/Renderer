#ifndef RENDER_RESOURCE_CREATE_INFO
#define RENDER_RESOURCE_CREATE_INFO
#include "render_resource.h"
namespace Render {
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

};

#endif