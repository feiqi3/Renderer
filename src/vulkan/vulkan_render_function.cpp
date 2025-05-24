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

    VkImageViewType toVkImageViewType(ImageType t)
    {
        switch (t) {
        case ImageType::V1D:        return VK_IMAGE_VIEW_TYPE_1D;
        case ImageType::V2D:        return VK_IMAGE_VIEW_TYPE_2D;
        case ImageType::V3D:        return VK_IMAGE_VIEW_TYPE_3D;
        case ImageType::VCube:       return VK_IMAGE_VIEW_TYPE_CUBE;
        case ImageType::V1D_Array:  return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
        case ImageType::V2D_Array:  return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        case ImageType::VCube_Array: return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
        default:                        return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
        }
    }

    VkImageType toVkImageType(ImageType t) {
        switch (t) {
        case ImageType::V1D:       
        case ImageType::V1D_Array: return VK_IMAGE_TYPE_1D;

        case ImageType::V2D:  
        case ImageType::VCube:
        case ImageType::VCube_Array:
        case ImageType::V2D_Array: return VK_IMAGE_TYPE_2D;

        case ImageType::V3D:       return VK_IMAGE_TYPE_3D;

        default:                   return VK_IMAGE_TYPE_2D;
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

        if ((type | BufferType::BufferType_Vertex)) {
            flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;      
        }
        if ( (type | BufferType::BufferType_Index)) {
            flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;       
        }
        if ( (type | BufferType::BufferType_Uniform)) {
            flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;   
        }
        if ((type | BufferType::BufferType_Storage)) {
            flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }
        if ( (type | BufferType::BufferType_TransferSrc)) {
            flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;        
        }
        if ( (type |  BufferType::BufferType_Indirect)) {
            flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;     
        }

        //For the every buffer except transfer add VK_BUFFER_USAGE_TRANSFER_DST_BIT
        if (!(type & BufferType::BufferType_TransferSrc)) {
            flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }

        return flags;
    }

    VkSampleCountFlagBits toVkSampleCount(SampleCount s) {
        switch (s) {
        case SampleCount::Count1:  return VK_SAMPLE_COUNT_1_BIT;
        case SampleCount::Count2:  return VK_SAMPLE_COUNT_2_BIT;
        case SampleCount::Count4:  return VK_SAMPLE_COUNT_4_BIT;
        case SampleCount::Count8:  return VK_SAMPLE_COUNT_8_BIT;
        default:                   return VK_SAMPLE_COUNT_1_BIT;
        }
    }

    VkImageUsageFlags toVkImageUsage(uint32_t u)
    {
        VkImageUsageFlags f = 0;
        if ( (u& ImageUsage::ImageUsage_Sampled))             f |= VK_IMAGE_USAGE_SAMPLED_BIT;
        if ( (u& ImageUsage::ImageUsage_Storage))             f |= VK_IMAGE_USAGE_STORAGE_BIT;
        if ( (u& ImageUsage::ImageUsage_TransferSrc))         f |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        if ( (u& ImageUsage::ImageUsage_TransferDst))         f |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        if ( (u& ImageUsage::ImageUsage_ColorAttachment))     f |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if ( (u& ImageUsage::ImageUsage_DepthStencilAttachment)) f |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        return f;
    }

    VkBorderColor toVkBorderColor(BorderColor bc)
    {
        switch (bc) {
        case BorderColor::FloatTransparentBlack: return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        case BorderColor::IntTransparentBlack:   return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
        case BorderColor::FloatOpaqueBlack:      return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        case BorderColor::IntOpaqueBlack:        return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        case BorderColor::FloatOpaqueWhite:      return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        case BorderColor::IntOpaqueWhite:        return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
        default:                                 return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        }
    }

    VkShaderStageFlags toVkShaderStageFlags(uint16_t stage)
    {
        VkShaderStageFlags flags = 0;

        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::Vertex))
            flags |= VK_SHADER_STAGE_VERTEX_BIT;
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::TessControl))
            flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::TessEvaluation))
            flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::Geometry))
            flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::Fragment))
            flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::Compute))
            flags |= VK_SHADER_STAGE_COMPUTE_BIT;
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::RayGen))
            flags |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::AnyHit))
            flags |= VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::ClosestHit))
            flags |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::Miss))
            flags |= VK_SHADER_STAGE_MISS_BIT_KHR;
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::Intersection))
            flags |= VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::Callable))
            flags |= VK_SHADER_STAGE_CALLABLE_BIT_KHR;
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::Task))
            flags |= VK_SHADER_STAGE_TASK_BIT_EXT;
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::Mesh))
            flags |= VK_SHADER_STAGE_MESH_BIT_EXT;

        return flags;
    }

    uint32_t findQueueFamily(rs_context_vk* ctx, QueueType type)
    {
        switch (type)
        {
        case Render::QueueType_Present:
        case Render::QueueType_Graphics:
            return ctx->graphicQueue->familyIndex;
            break;
        case Render::QueueType_Compute:
            if (ctx->computeQueue) {
                return ctx->computeQueue->familyIndex;
            }
            else { 
                return ctx->graphicQueue->familyIndex;
            }
            break;
        case Render::QueueType_Transfer:
            if (ctx->transferQueue) {
                return ctx->transferQueue->familyIndex;
            }
            else {
                return ctx->graphicQueue->familyIndex;
            }
            break;
        default:
            return ctx->graphicQueue->familyIndex;
            break;
        }
    }

    VkCompareOp toVkCompareOp(CompareOp op)
    {
        switch (op) {
        case CompareOp::Never:          return VK_COMPARE_OP_NEVER;
        case CompareOp::Less:           return VK_COMPARE_OP_LESS;
        case CompareOp::Equal:          return VK_COMPARE_OP_EQUAL;
        case CompareOp::LessOrEqual:    return VK_COMPARE_OP_LESS_OR_EQUAL;
        case CompareOp::Greater:        return VK_COMPARE_OP_GREATER;
        case CompareOp::NotEqual:       return VK_COMPARE_OP_NOT_EQUAL;
        case CompareOp::GreaterOrEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case CompareOp::Always:         return VK_COMPARE_OP_ALWAYS;
        default:                        return VK_COMPARE_OP_ALWAYS;
        }
    }

    VkFrontFace toVkFrontFace(FrontFace face)
    {
        switch (face)
        {
        case Render::FrontFace::ClockWise:
            return VK_FRONT_FACE_CLOCKWISE;
            break;
        case Render::FrontFace::CtClockWise:
            return VK_FRONT_FACE_COUNTER_CLOCKWISE;
                break;
        default:
            return VK_FRONT_FACE_CLOCKWISE;
            break;
        }
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
            if (desc.bufUsage & BufferType::BufferType_TransferSrc) {
                vmaFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            }
            else {
                vmaFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            }
        }

        VmaAllocationCreateInfo ai{};
        ai.flags = vmaFlags;
        ai.usage = VMA_MEMORY_USAGE_AUTO;
        VK_CHECK(vmaCreateBuffer(context->allocator, &CI, &ai, &buffer, &allocation, &allocInfo),
            {
                return nullptr;
            }
            );
    
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

    rs_image_vk* createRsImage(rs_context_vk* ctx, ImageDesc& desc)
    {
        VkImage image;
        VkImageView imageview;
        VmaAllocation alloc;
        VkImageCreateInfo ici{};
        ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ici.pNext = nullptr;
        ici.flags = 0;
        ici.imageType = toVkImageType(desc.type);
        ici.format = toVkFormat(desc.format);
        ici.extent = { desc.width, desc.height, desc.depth };
        ici.mipLevels = desc.mipLevels;
        ici.arrayLayers = desc.arrayLayers;
        ici.samples =toVkSampleCount(desc.samples);
        ici.tiling = VK_IMAGE_TILING_OPTIMAL;
        ici.usage = toVkImageUsage(desc.usage);
        ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo ai{};
        ai.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        VmaAllocationInfo aif;
        vmaCreateImage(ctx->allocator, &ici, &ai, &image, &alloc, &aif);

        VkImageViewCreateInfo ivci{};
        ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ivci.image = image;
        ivci.viewType = toVkImageViewType(desc.type);
        ivci.format = ici.format;
        ivci.components = { VK_COMPONENT_SWIZZLE_IDENTITY };
        ivci.subresourceRange.aspectMask =
            (desc.usage& ImageUsage_DepthStencilAttachment)
            ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
            : VK_IMAGE_ASPECT_COLOR_BIT;
        ivci.subresourceRange.baseMipLevel = 0;
        ivci.subresourceRange.levelCount = desc.mipLevels;
        ivci.subresourceRange.baseArrayLayer = 0;
        ivci.subresourceRange.layerCount = desc.arrayLayers;

        VK_CHECK(vkCreateImageView(ctx->device, &ivci, nullptr, &imageview),
        {
            return nullptr;
        }
       );


        auto ret = new rs_image_vk;
        ret->native = image;
        ret->view = imageview;
        ret->allocation = alloc;
        ret->width = desc.width;
        ret->height = desc.height;
        ret->depth = desc.depth;
        ret->type = desc.type;
        ret->format = desc.format;
        ret->mipLevels = desc.mipLevels;
        ret->arrayLayers = desc.arrayLayers;
        return ret;
    }

    rs_sampler_vk* createRsSampler(rs_context_vk* ctx, SamplerDesc& desc)
    {
        VkSamplerCreateInfo sci{};
        sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sci.pNext = nullptr;
        sci.addressModeU = toVkAddressMode(desc.addressU);
        sci.addressModeV = toVkAddressMode(desc.addressV);
        sci.addressModeW = toVkAddressMode(desc.addressW);
        sci.magFilter = toVkFilter(desc.magFilter);
        sci.minFilter = toVkFilter(desc.minFilter);
        sci.mipmapMode = toVkMipmapFilterMode(desc.mipmapMode);
        sci.mipLodBias = 0.f;
        sci.anisotropyEnable = desc.enableAnisotropy ? VK_TRUE : VK_FALSE;
        sci.maxAnisotropy = desc.maxAnisotropy;
        sci.minLod = 0;
        sci.maxLod = VK_LOD_CLAMP_NONE;
        sci.compareEnable = desc.enableCompare ? VK_TRUE : VK_FALSE;
        sci.compareOp = desc.enableCompare ? toVkCompareOp(desc.compareOp)
            : VK_COMPARE_OP_NEVER;
        sci.unnormalizedCoordinates = desc.unnormalizedCoords ? VK_TRUE : VK_FALSE;
        sci.borderColor = toVkBorderColor(desc.borderColor);

        rs_sampler_vk* result = new rs_sampler_vk();
        VkSampler sampler;
        VK_CHECK(vkCreateSampler(ctx->device, &sci, nullptr, &sampler),
            { delete result; return nullptr; }
        );
        result->native = sampler;

        return result;
    }

    rs_shader_module_vk* createRsShader(rs_context_vk* context, ShaderDesc& desc)
    {
        VkShaderModuleCreateInfo ci{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        ci.codeSize = desc.codeSizeByte;
        ci.pCode = reinterpret_cast<const uint32_t*>(desc.shaderCode);

        VkShaderModule smodule;
        VK_CHECK(vkCreateShaderModule(context->device, &ci, 0, &smodule),
            {
            return nullptr;
            }
        );
        rs_shader_module_vk* shader = new rs_shader_module_vk();
        shader->shaderStage = desc.stage;
        shader->native = smodule;
        shader->entryPoint = desc.entryPoint;
        return shader;
    }

    rs_commandbuffer_vk* createRsCommand(rs_context_vk* ctx, const CommandBufferDesc& desc)
    {
        VkCommandPool pool = VK_NULL_HANDLE;
        {
            VkCommandPoolCreateInfo pci{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
            pci.queueFamilyIndex = findQueueFamily(ctx, desc.queueType);
            pci.flags = desc.transient ? VK_COMMAND_POOL_CREATE_TRANSIENT_BIT : 0;
            VK_CHECK(
                vkCreateCommandPool(ctx->device, &pci, nullptr, &pool),
                {
                    return nullptr;
                }
            );
        }

        // 分配命令缓冲
        VkCommandBufferAllocateInfo ai{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        ai.commandPool = pool;
        ai.level = desc.isSecondary
            ? VK_COMMAND_BUFFER_LEVEL_SECONDARY
            : VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = 1;
        
        VkCommandBuffer cmd = VK_NULL_HANDLE;
        if (vkAllocateCommandBuffers(ctx->device, &ai, &cmd) != VK_SUCCESS) {
            vkDestroyCommandPool(ctx->device, pool, nullptr);
            return nullptr;
        }

        auto cb = new rs_commandbuffer_vk();
        cb->pool = pool;
        cb->native = cmd;
        cb->isSecondary = desc.isSecondary;
        cb->isTransitent = desc.transient;
        return cb;
    }

}


