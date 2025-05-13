#ifndef RENDER_RESOURCE_DEF_H_
#define RENDER_RESOURCE_DEF_H_
#include <stdint.h>
namespace Render {
    enum BufferType : uint32_t {
        None = 0,
        Vertex = 1u << 0,  
        Index = 1u << 1, 
        Uniform = 1u << 2,   
        Storage = 1u << 3, 
        Transfer = 1u << 4,  //Means GPU visible
        Indirect = 1u << 5, 
        Count = 1u << 6,
    };

    enum class ShaderType : uint32_t {
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

        // 16-bit normalized or float color formats
        R16_UNORM,          // 16-bit R                
        RG16_UNORM,         // 16-bit R, G             
        RGBA16_UNORM,       // 16-bit R, G, B, A        
        R16_SFLOAT,         // 16-bit float R          
        RG16_SFLOAT,        // 16-bit float R, G        
        RGBA16_SFLOAT,      // 16-bit float RGBA        

        // 32-bit float color formats (HDR)
        R32_SFLOAT,         // 32-bit float R           
        RG32_SFLOAT,        // 32-bit float R, G       
        RGBA32_SFLOAT,      // 32-bit float RGBA        

        // Depth/stencil formats
        D16_UNORM,          // 16-bit depth             
        D24_UNORM_S8_UINT,  // 24-bit depth + 8-bit S   
        D32_SFLOAT,         // 32-bit float depth       
        D32_SFLOAT_S8_UINT, // 32-bit float depth + 8-bit S 

        Invalid,
    };

    enum class ImageViewType : uint8_t {
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
        Graphics = 1 << 0,
        Compute = 1 << 1,
        Transfer = 1 << 2,
        Present = 1 << 3,
    };

    enum class MemoryUsage : uint8_t {
        Unknown = 0, 
        GPUOnly,     
        CPUOnly,  
        CPUToGPU,     
        GPUToCPU,    
        CPUToGPU_Cached,   
        GPUToCPU_Cached  
    };
};

#endif