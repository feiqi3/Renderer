#ifndef RENDER_RESOURCE_DEF_H_
#define RENDER_RESOURCE_DEF_H_
#include <stdint.h>
#include <array>
namespace Render {
    enum BufferType : uint32_t {
        BufferType_None = 0,
        BufferType_Vertex = 1u << 0,  
        BufferType_Index = 1u << 1, 
        BufferType_Uniform = 1u << 2,   
        BufferType_Storage = 1u << 3, 
        BufferType_TransferSrc = 1u << 4,
        BufferType_Indirect = 1u << 5, 
        BufferType_Count = 1u << 6,
    };

    enum ImageUsage : uint32_t {
        ImageUsage_None = 0,
        ImageUsage_Sampled = 1 << 0,
        ImageUsage_Storage = 1 << 1,
        ImageUsage_TransferSrc = 1 << 2,
        ImageUsage_TransferDst = 1 << 3,
        ImageUsage_ColorAttachment = 1 << 4,
        ImageUsage_DepthStencilAttachment = 1 << 5,
        ImageUsage_PresentSrc = 1 << 6,
        ImageUsage_Count = 1 << 7,
    };

    enum class ShaderStage : uint16_t {
        None = 0,

        Vertex = 1 << 0, 
        TessControl = 1 << 1, 
        TessEvaluation = 1 << 2,
        Geometry = 1 << 3, 
        Fragment = 1 << 4,

        Compute = 1 << 5, 

        RayGen = 1 << 6, 
        AnyHit = 1 << 7,  
        ClosestHit = 1 << 8,  
        Miss = 1 << 9, 
        Intersection = 1 << 10,
        Callable = 1 << 11,

        Task = 1 << 12,  
        Mesh = 1 << 13,
    };

    enum class Filter : uint8_t {
        Nearest,
        Linear,
        Cubic,
        Unknown
    };

    using MipMapMode = Filter;

    enum class ImageFormat : uint32_t {
        Unknown = 0,

        // 8-bit normalized color formats
        R8_UNORM,           // 8-bit R                
        RG8_UNORM,          // 8-bit R, G              
        RGB8_UNORM,         // 8-bit R, G, B            
        RGBA8_UNORM,        // 8-bit R, G, B, A         
        BGRA8_UNORM,        // 8-bit B, G, R, A        

        // 8-bit sRGB color formats
        SRGB8,              // 8-bit sRGB             
        SRGB8_ALPHA8,       // 8-bit sRGB + alpha       
        SBGR8_ALPHA8,       // 8-bit sBGR + alpha       

        // 16-bit normalized or float color formats
        R16_UNORM,          // 16-bit R                
        RG16_UNORM,         // 16-bit R, G             
        RGB16_UNORM,         // 16-bit R, G, B             
        RGBA16_UNORM,       // 16-bit R, G, B, A        
        R16_SFLOAT,         // 16-bit float R          
        RG16_SFLOAT,        // 16-bit float R, G        
        RGB16_SFLOAT,       // 16-bit float R, G, B        
        RGBA16_SFLOAT,      // 16-bit float RGBA        

        // 32-bit float color formats (HDR)
        R32_SFLOAT,         // 32-bit float R           
        RG32_SFLOAT,        // 32-bit float R, G       
        RGB32_SFLOAT,        // 32-bit float R, G, B       
        RGBA32_SFLOAT,      // 32-bit float RGBA        

        // 32-bit integer color formats 
        RGBA32_UINT,

        // Depth/stencil formats
        D16_UNORM,          // 16-bit depth             
        D24_UNORM_S8_UINT,  // 24-bit depth + 8-bit S   
        D32_SFLOAT,         // 32-bit float depth       
        D32_SFLOAT_S8_UINT, // 32-bit float depth + 8-bit S 

        Invalid,
    };

    using FormatCapFlag = uint8_t;
    namespace ImgFormatCaps {
        inline FormatCapFlag Supported     = 1 << 0;
        inline FormatCapFlag ColorAtt      = 1 << 1;
        inline FormatCapFlag DepthStencil  = 1 << 2;
        inline FormatCapFlag Sample        = 1 << 3;
        inline FormatCapFlag Storage       = 1 << 4;
    };

    enum class RenderTextureFormat : uint8_t
    {
        // LDR Color
        RGBA8,          // 8-8-8-8 UNORM

        // HDR Color
        R16F,           // 1 channel
        RG16F,          // 2 channel
        RGBA16F,        // 4 channel
        RGBA32F,        // 4 channel

        // Depth / Stencil
        D24S8,          // packed depth-stencil
        D32,            // Pure depth
        SwapchainFormat,

        Invalid
    };

    enum class VertexFormat : uint8_t
    {
        Float,
        Float2,
        Float3,
        Float4,

        Half2,
        Half4,

        Uint4,   // bone indices
        UByte4N, // normalized color / weights

        Invalid,
    };

    enum class ImageType : uint8_t {
        V1D,    
        V2D,        
        V3D,     
        VCube,     
        V1D_Array, 
        V2D_Array,   
        VCube_Array, 
        Invalid      
    };

    enum class AddressMode : uint8_t {
        Repeat,             
        MirroredRepeat,     
        ClampToEdge,       
        ClampToBorder,     
        MirrorClampToEdge,  
        Unknown             
    };

    enum QueueType : uint8_t {
        QueueType_Graphics = 1 << 0,
        QueueType_Compute = 1 << 1,
        QueueType_Transfer = 1 << 2,
        QueueType_Present = 1 << 3,
    };

    enum class SampleCount : uint8_t {
        Count1 = 0,   // No MSAA
        Count2 = 1,   // 2x MSAA
        Count4 = 2,   // 4x MSAA
        Count8 = 3,   // 8x MSAA
        Invalid   ,
    };

    enum class BorderColor : uint8_t {
        FloatTransparentBlack,  // (0,0,0,0)
        IntTransparentBlack,    // (0,0,0,0) as integer
        FloatOpaqueBlack,       // (0,0,0,1)
        IntOpaqueBlack,         // (0,0,0,1) as integer
        FloatOpaqueWhite,       // (1,1,1,1)
        IntOpaqueWhite          // (1,1,1,1) as integer
    };

    enum class PipelineType {
        Graphics, 
        Compute,  
        RayTracing 
    };

    // 深度／模板比较方式
    enum class CompareOp : uint8_t {
        Never,
        Less,
        Equal,
        LessOrEqual,
        Greater,
        NotEqual,
        GreaterOrEqual,
        Always
    };

    // 混合因子和操作
    enum class BlendFactor : uint8_t {
        Zero,
        One,
        SrcColor,
        OneMinusSrcColor,
        DstColor,
        OneMinusDstColor,
        SrcAlpha,
        OneMinusSrcAlpha,
        DstAlpha,
        OneMinusDstAlpha
    };
    enum class BlendOp : uint8_t {
        Add,
        Subtract,
        ReverseSubtract,
        Min,
        Max
    };

    enum class Topology {
        TriangleList,
        TriangleStrip,
        TriangleFAn,
        Point,
        LineList,
        LineStrip,
    };

    enum class FillMode {
        Fill,
        Line,
        Point,
    };

    enum class CullMode {
        None,
        Front,
        Back,
        FrontAndBack
    };

    enum class FrontFace {
        ClockWise,
        CtClockWise,
    };

    enum class StencilOp : uint8_t {
        Keep,                  // 保持原值
        Zero,                  // 置零
        Replace,               // 用参考值替换
        IncrementAndClamp,     // 增量并钳制到最大值
        DecrementAndClamp,     // 减量并钳制到 0
        Invert,                // 按位取反
        IncrementAndWrap,      // 增量并溢出回绕到 0
        DecrementAndWrap       // 减量并溢出回绕到最大值
    };

    enum class UniformType : uint8_t {
        ConstantBuffer, //A very important convention: those buffer begin with "CBUFFER_" will be marked as ConstantBuffer
        UniformBuffer,
        StorageBuffer,
        StorageImage,
        Texture,
        InputAttachment,
        Sampler,
//        AccelerationStructure,
        Count
    };

    enum class IndexType : uint8_t {
        Uint16,
        Uint32
    };

    struct ClearColor {
        float rgba[4];
    };

    struct ClearDepthStencil {
        float depth;
        uint32_t stencil;
    };

    enum class ViewAspect {
        Depth               = 1,
        Stencil             = 2,
        DepthAndStencil     = 3,
        Color               = 4,
        Invalid,
    };

    enum class ShaderLang {
        HLSL,
        GLSL,
        MSL
    };

    enum class UAVAccess : uint8_t {
        ReadOnly,
        WriteOnly,
        ReadWrite
	};

    enum class ResourceState : uint64_t {
        // ==========================================
        // Initial State
        // ==========================================
        Common = 0,          // Undefined

        // ==========================================
        // For Buffer
        // ==========================================
        VertexBuffer = 1ULL << 0,  // Read as Vertex Buffer
        IndexBuffer = 1ULL << 1,  //  Read as Index Buffer
        UniformBuffer = 1ULL << 2,  // Read as Uniform Buffer
        IndirectArgument = 1ULL << 3,  // Read as  Indirect Buffer

        // ==========================================
        // Image & Buffer
        // ==========================================
        ShaderResource = 1ULL << 4,  // Shader Read (Sampled Image / Texture)
        UnorderedAccess = 1ULL << 5,  // Shader Read/Write (Storage Image / Storage Buffer / UAV)

        ComputeShaderResource = 1ULL << 6, //Compute Shader Read (Sampled Image / Texture)
        ComputeUnorderedAccess = 1ULL << 7, //Compute Shader Read/Write (Storage Image / Storage Buffer / UAV)
        // ==========================================
        // Render Target
        // ==========================================
        RenderTarget = 1ULL << 8,  // Color Attachment
        DepthStencilWrite = 1ULL << 9,  // Write Depth Stencil Attachment
        DepthStencilRead = 1ULL << 10,  // Read Depth Stencil Attachment

        // ==========================================
        // Copy / Blit / Resolve
        // ==========================================
        TransferSrc = 1ULL << 11,  
        TransferDst = 1ULL << 12,
        ResolveSrc = 1ULL << 13, 
        ResolveDst = 1ULL << 14, 

        // ==========================================
        // Host 
        // ==========================================
        HostRead = 1ULL << 15, // VK_ACCESS_HOST_READ_BIT
        HostWrite = 1ULL << 16, // VK_ACCESS_HOST_WRITE_BIT

        // ==========================================
        // 特殊状态
        // ==========================================
        Present = 1ULL << 17, 
        GenericRead = 1ULL << 18,
        General = 1ULL << 19  
    };

    using rs_binding_pos = uint32_t;

	using DrawDataArray = std::array<struct rs_drawdata*, 4>;

#define INVALID_BINDLESS_INDEX 0xFFFFFFFF
};

#endif