#include "vulkan/vulkan_render_function.h"
#include "bit_helper.h"
namespace Render::Vulkan {
    VkFormat toVkFormat(ImageFormat fmt)
    {
        switch (fmt) {
        case ImageFormat::R8_UNORM:           return VK_FORMAT_R8_UNORM;
        case ImageFormat::RG8_UNORM:          return VK_FORMAT_R8G8_UNORM;
        case ImageFormat::RGB8_UNORM:         return VK_FORMAT_R8G8B8_UNORM;
        case ImageFormat::RGBA8_UNORM:        return VK_FORMAT_R8G8B8A8_UNORM;
        case ImageFormat::BGRA8_UNORM:        return VK_FORMAT_B8G8R8A8_UNORM;

        case ImageFormat::SRGB8:              return VK_FORMAT_R8_SRGB;
        case ImageFormat::SRGB8_ALPHA8:       return VK_FORMAT_R8G8B8A8_SRGB;

        case ImageFormat::R16_UNORM:          return VK_FORMAT_R16_UNORM;
        case ImageFormat::RG16_UNORM:         return VK_FORMAT_R16G16_UNORM;
        case ImageFormat::RGBA16_UNORM:       return VK_FORMAT_R16G16B16A16_UNORM;
        case ImageFormat::R16_SFLOAT:         return VK_FORMAT_R16_SFLOAT;
        case ImageFormat::RG16_SFLOAT:        return VK_FORMAT_R16G16_SFLOAT;
        case ImageFormat::RGBA16_SFLOAT:      return VK_FORMAT_R16G16B16A16_SFLOAT;

        case ImageFormat::R32_SFLOAT:         return VK_FORMAT_R32_SFLOAT;
        case ImageFormat::RG32_SFLOAT:        return VK_FORMAT_R32G32_SFLOAT;
        case ImageFormat::RGBA32_SFLOAT:      return VK_FORMAT_R32G32B32A32_SFLOAT;

        case ImageFormat::D16_UNORM:          return VK_FORMAT_D16_UNORM;
        case ImageFormat::D24_UNORM_S8_UINT:  return VK_FORMAT_D24_UNORM_S8_UINT;
        case ImageFormat::D32_SFLOAT:         return VK_FORMAT_D32_SFLOAT;
        case ImageFormat::D32_SFLOAT_S8_UINT: return VK_FORMAT_D32_SFLOAT_S8_UINT;

        default:                              return VK_FORMAT_UNDEFINED;
        }
    }

    //DXGI_FORMAT toDxgiFormat(ImageFormat fmt) {
    //    switch (fmt) {
    //    case ImageFormat::R8_UNORM:           return DXGI_FORMAT_R8_UNORM;                // :contentReference[oaicite:45]{index=45}
    //    case ImageFormat::RG8_UNORM:          return DXGI_FORMAT_R8G8_UNORM;              // :contentReference[oaicite:46]{index=46}
    //    case ImageFormat::RGB8_UNORM:         return DXGI_FORMAT_UNKNOWN; /* Not supported */ // :contentReference[oaicite:47]{index=47}
    //    case ImageFormat::RGBA8_UNORM:        return DXGI_FORMAT_R8G8B8A8_UNORM;          // :contentReference[oaicite:48]{index=48}
    //    case ImageFormat::BGRA8_UNORM:        return DXGI_FORMAT_B8G8R8A8_UNORM;          // :contentReference[oaicite:49]{index=49}

    //    case ImageFormat::SRGB8:              return DXGI_FORMAT_R8_UNORM_SRGB;           // :contentReference[oaicite:50]{index=50}
    //    case ImageFormat::SRGB8_ALPHA8:       return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;     // :contentReference[oaicite:51]{index=51}

    //    case ImageFormat::R16_UNORM:          return DXGI_FORMAT_R16_UNORM;               // :contentReference[oaicite:52]{index=52}
    //    case ImageFormat::RG16_UNORM:         return DXGI_FORMAT_R16G16_UNORM;            // :contentReference[oaicite:53]{index=53}
    //    case ImageFormat::RGBA16_UNORM:       return DXGI_FORMAT_R16G16B16A16_UNORM;      // :contentReference[oaicite:54]{index=54}
    //    case ImageFormat::R16_SFLOAT:         return DXGI_FORMAT_R16_FLOAT;               // :contentReference[oaicite:55]{index=55}
    //    case ImageFormat::RG16_SFLOAT:        return DXGI_FORMAT_R16G16_FLOAT;            // :contentReference[oaicite:56]{index=56}
    //    case ImageFormat::RGBA16_SFLOAT:      return DXGI_FORMAT_R16G16B16A16_FLOAT;      // :contentReference[oaicite:57]{index=57}

    //    case ImageFormat::R32_SFLOAT:         return DXGI_FORMAT_R32_FLOAT;               // :contentReference[oaicite:58]{index=58}
    //    case ImageFormat::RG32_SFLOAT:        return DXGI_FORMAT_R32G32_FLOAT;            // :contentReference[oaicite:59]{index=59}
    //    case ImageFormat::RGBA32_SFLOAT:      return DXGI_FORMAT_R32G32B32A32_FLOAT;      // :contentReference[oaicite:60]{index=60}

    //    case ImageFormat::D16_UNORM:          return DXGI_FORMAT_D16_UNORM;               // :contentReference[oaicite:61]{index=61}
    //    case ImageFormat::D24_UNORM_S8_UINT:  return DXGI_FORMAT_D24_UNORM_S8_UINT;       // :contentReference[oaicite:62]{index=62}
    //    case ImageFormat::D32_SFLOAT:         return DXGI_FORMAT_D32_FLOAT;               // :contentReference[oaicite:63]{index=63}
    //    case ImageFormat::D32_SFLOAT_S8_UINT: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;     // :contentReference[oaicite:64]{index=64}

    //    default:                              return DXGI_FORMAT_UNKNOWN;                 // :contentReference[oaicite:65]{index=65}
    //    }
    //}

    VkImageViewType toVkImageViewType(ImageViewType t)
    {
        switch (t) {
        case ImageViewType::V1D:        return VK_IMAGE_VIEW_TYPE_1D;
        case ImageViewType::V2D:        return VK_IMAGE_VIEW_TYPE_2D;
        case ImageViewType::V3D:        return VK_IMAGE_VIEW_TYPE_3D;
        case ImageViewType::VCube:       return VK_IMAGE_VIEW_TYPE_CUBE;
        case ImageViewType::V1D_Array:  return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
        case ImageViewType::V2D_Array:  return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        case ImageViewType::VCube_Array: return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
        default:                        return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
        }
    }
    VkSamplerAddressMode toVkAddressMode(AddressMode mode)
    {
        switch (mode) {
        case AddressMode::Repeat:            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case AddressMode::MirroredRepeat:    return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case AddressMode::ClampToEdge:       return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case AddressMode::ClampToBorder:     return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case AddressMode::MirrorClampToEdge: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
        default:                             return VK_SAMPLER_ADDRESS_MODE_REPEAT; // 默认回退
        }
    }

    VkFilter toVkFilter(Filter f) {
        switch (f) {
            case Filter::Nearest: return VK_FILTER_NEAREST;   
            case Filter::Linear:  return VK_FILTER_LINEAR;      
            case Filter::Cubic:   return VK_FILTER_CUBIC_EXT;    
            default:              return VK_FILTER_NEAREST;
        }
    }

    VkSamplerMipmapMode toVkMipmapFilterMode(Filter m)
    {
        switch (m) {
            case Filter::Nearest: return VK_SAMPLER_MIPMAP_MODE_NEAREST; 
            case Filter::Linear:  return VK_SAMPLER_MIPMAP_MODE_LINEAR; 
            default:                  return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        }
    }

    VkBufferUsageFlags toVkBufferUsageFlags(uint32_t type)
    {
        VkBufferUsageFlags flags = 0;

        if ((type | BufferType::Vertex)) {
            flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;      
        }
        if ( (type | BufferType::Index)) {
            flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;       
        }
        if ( (type | BufferType::Uniform)) {
            flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;   
        }
        if ((type | BufferType::Storage)) {
            flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }
        if ( (type | BufferType::Transfer)) {
            flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;        
        }
        if ( (type |  BufferType::Indirect)) {
            flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;     
        }

        //For the every buffer except transfer add VK_BUFFER_USAGE_TRANSFER_DST_BIT
        if (!(type & BufferType::Transfer)) {
            flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }

        return flags;
    }

    rs_buffer_vk* createRsBuffer(rs_context_vk* context, BufferDesc& desc)
    {
        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo allocInfo;

        auto toUseQueue = desc.queueType;
        assert(Util::ContainsAll(context->graphicQueue->queueType, toUseQueue));
        uint32_t queueFamily = context->graphicQueue->familyIndex;
        VkBufferCreateInfo CI{};
        CI.        sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        CI. pNext = nullptr;
        CI.    flags = 0;
        CI.           size = desc.byteSize;
        CI.     usage = toVkBufferUsageFlags(desc.bufUsage);
        CI.          sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        CI. queueFamilyIndexCount = 1;
        CI. pQueueFamilyIndices = &queueFamily;
        
        uint32_t vmaFlags = 0;
        if (desc.mappable) {
            if (desc.bufUsage & BufferType::Transfer) {
                vmaFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            }
            else {
                vmaFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            }
        }

        VmaAllocationCreateInfo ai{};
        ai.flags = vmaFlags;
        ai.usage = VMA_MEMORY_USAGE_AUTO;
        vmaCreateBuffer(context->allocator, &CI, &ai, &buffer, &allocation, &allocInfo);
    
        rs_buffer_vk* ret = new rs_buffer_vk;
        ret->allocation = allocation;
        ret->bufferType = desc.bufUsage;
        ret->byteSize = desc.byteSize;
        ret->native = buffer;

        return ret;
    }

    void* mapRsBuffer(rs_context_vk* context, rs_buffer_vk* buffer)
    {
        assert(isRsBufferMappable(context, buffer));
        void* mappedData = nullptr;
        VkResult result = vmaMapMemory(context->allocator, buffer->allocation, &mappedData);
        if (result != VK_SUCCESS) {
            return nullptr;
        }
        return mappedData;
    }

    void unmapRsBuffer(rs_context_vk* context, rs_buffer_vk* buffer)
    {
        assert(isRsBufferMappable(context,buffer));
        vmaUnmapMemory(context->allocator, buffer->allocation);
    }

    bool isRsBufferMappable(rs_context_vk* context, rs_buffer_vk* buffer)
    {
        VkMemoryPropertyFlags memFlags;
        vmaGetAllocationMemoryProperties(context->allocator, buffer->allocation, &memFlags);
        return (memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;
    }

    enum class Filter : uint8_t {
        Nearest,     
        Linear,      
        Cubic,    
        Unknown  
    };
}


