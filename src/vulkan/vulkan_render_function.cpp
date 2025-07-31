#define VK_NO_PROTOTYPES
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION
#include "volk.h"
#include "vk_mem_alloc.h"
#include "GLFW/glfw3.h"
#include "vulkan/vulkan_shader_reflect.h"
#include "window/render_resource_window_glfw.h"
#include "vulkan/vulkan_render_function.h"
#include "bit_helper.h"
#include "vulkan/vulkan_descriptor_set.h"
#include "vulkan/vulkan_command.h"
#include "vulkan/vulkan_deferred_destroy.h"


#include "render_log.h"
#include <set>
#include <iostream>
namespace {
    //validation callback
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT              messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {
        switch (messageTypes)
        {case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT :
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT :
            Render::Log::info(pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            Render::Log::warn(pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        default:
            Render::Log::error(pCallbackData->pMessage);
            break;
        }
        return VK_FALSE;
    }

    std::vector<VkPresentModeKHR> querySwapchainPresentModes(
        VkPhysicalDevice   physicalDevice,
        VkSurfaceKHR       surface)
    {
        // 1. 拿到可用模式数量
        uint32_t modeCount = 0;
        VkResult res = vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice,
            surface,
            &modeCount,
            nullptr);
        if (res != VK_SUCCESS || modeCount == 0) {
            uint64_t _ = 0;
            (void)*((int*)_);
        }

        // 2. 分配数组并再次调用填充数据
        std::vector<VkPresentModeKHR> modes(modeCount);
        res = vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice,
            surface,
            &modeCount,
            modes.data());
        if (res != VK_SUCCESS) {
            uint64_t _ = 0;
            (void)*((int*)_);
        }

        return modes;
    }

    std::vector<VkSurfaceFormatKHR> querySwapchainFormats(
        VkPhysicalDevice   physicalDevice,
        VkSurfaceKHR       surface)
    {
        // 1. 先拿可用格式数量
        uint32_t formatCount = 0;
        VkResult res = vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice,
            surface,
            &formatCount,
            nullptr);
        if (res != VK_SUCCESS || formatCount == 0) {
            uint64_t _ = 0;
            (void)*((int*)_);
        }

        // 2. 分配数组并再次调用填充数据
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        res = vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice,
            surface,
            &formatCount,
            formats.data());
        if (res != VK_SUCCESS) {
            uint64_t _ = 0;
            (void)*((int*)_);
        }

        return formats;
    }

    uint32_t chooseSwapchainImageCount(const VkSurfaceCapabilitiesKHR& caps) {
        // 在 minImageCount 基础上加 1，实现“至少双缓冲+一个” → 三缓冲
        uint32_t desired = caps.minImageCount + 1;
        // 如果 maxImageCount 非 0（有限制），则取二者最小
        if (caps.maxImageCount > 0) {
            desired = std::min(desired, caps.maxImageCount);
        }
        return desired;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& fmt : availableFormats) {
            if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB &&
                fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return fmt;
            }
        }
        // fallback
        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availableModes) {
        for (auto mode : availableModes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return mode;
            }
        }
        for (auto mode : availableModes) {
            if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                return mode;
            }
        }
        // fallback
        return VK_PRESENT_MODE_FIFO_KHR;
    }
}

namespace Render::Vulkan {


    ImageFormat ToImageFormat(VkFormat format) {
        switch (format) {
        case VK_FORMAT_R8_UNORM:                return ImageFormat::R8_UNORM;
        case VK_FORMAT_R8G8_UNORM:              return ImageFormat::RG8_UNORM;
        case VK_FORMAT_R8G8B8_UNORM:            return ImageFormat::RGB8_UNORM;
        case VK_FORMAT_R8G8B8A8_UNORM:          return ImageFormat::RGBA8_UNORM;
        case VK_FORMAT_B8G8R8A8_UNORM:          return ImageFormat::BGRA8_UNORM;
        case VK_FORMAT_B8G8R8A8_SRGB:          return ImageFormat::SBGR8_ALPHA8;

        case VK_FORMAT_R8G8B8_SRGB:             return ImageFormat::SRGB8;
        case VK_FORMAT_R8G8B8A8_SRGB:           return ImageFormat::SRGB8_ALPHA8;

        case VK_FORMAT_R16_UNORM:               return ImageFormat::R16_UNORM;
        case VK_FORMAT_R16G16_UNORM:            return ImageFormat::RG16_UNORM;
        case VK_FORMAT_R16G16B16_UNORM:         return ImageFormat::RGB16_UNORM;
        case VK_FORMAT_R16G16B16A16_UNORM:      return ImageFormat::RGBA16_UNORM;

        case VK_FORMAT_R16_SFLOAT:              return ImageFormat::R16_SFLOAT;
        case VK_FORMAT_R16G16_SFLOAT:           return ImageFormat::RG16_SFLOAT;
        case VK_FORMAT_R16G16B16_SFLOAT:           return ImageFormat::RGB16_SFLOAT;
        case VK_FORMAT_R16G16B16A16_SFLOAT:     return ImageFormat::RGBA16_SFLOAT;

        case VK_FORMAT_R32_SFLOAT:              return ImageFormat::R32_SFLOAT;
        case VK_FORMAT_R32G32_SFLOAT:           return ImageFormat::RG32_SFLOAT;
        case VK_FORMAT_R32G32B32_SFLOAT:           return ImageFormat::RGB32_SFLOAT;
        case VK_FORMAT_R32G32B32A32_SFLOAT:     return ImageFormat::RGBA32_SFLOAT;

        case VK_FORMAT_D16_UNORM:               return ImageFormat::D16_UNORM;
        case VK_FORMAT_D24_UNORM_S8_UINT:       return ImageFormat::D24_UNORM_S8_UINT;
        case VK_FORMAT_D32_SFLOAT:              return ImageFormat::D32_SFLOAT;
        case VK_FORMAT_D32_SFLOAT_S8_UINT:      return ImageFormat::D32_SFLOAT_S8_UINT;

        case VK_FORMAT_UNDEFINED:               return ImageFormat::Unknown;

        default:                                return ImageFormat::Invalid;
        }
    }


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
        case ImageFormat::SBGR8_ALPHA8:       return VK_FORMAT_B8G8R8A8_SRGB;

        case ImageFormat::R16_UNORM:          return VK_FORMAT_R16_UNORM;
        case ImageFormat::RG16_UNORM:         return VK_FORMAT_R16G16_UNORM;
        case ImageFormat::RGB16_UNORM:         return VK_FORMAT_R16G16B16_UNORM;
        case ImageFormat::RGBA16_UNORM:       return VK_FORMAT_R16G16B16A16_UNORM;
        case ImageFormat::R16_SFLOAT:         return VK_FORMAT_R16_SFLOAT;
        case ImageFormat::RG16_SFLOAT:        return VK_FORMAT_R16G16_SFLOAT;
        case ImageFormat::RGB16_SFLOAT:        return VK_FORMAT_R16G16B16_SFLOAT;
        case ImageFormat::RGBA16_SFLOAT:      return VK_FORMAT_R16G16B16A16_SFLOAT;

        case ImageFormat::R32_SFLOAT:         return VK_FORMAT_R32_SFLOAT;
        case ImageFormat::RG32_SFLOAT:        return VK_FORMAT_R32G32_SFLOAT;
        case ImageFormat::RGB32_SFLOAT:        return VK_FORMAT_R32G32B32_SFLOAT;
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

    rs_context_vk* initVulkanBackEnd(BackEndInitDesc& desc, Window::rs_window* window)
    {
        rs_context_vk* ctx = new rs_context_vk;
        ctx->initDesc = desc;
        createVkInstance(ctx);
        createSurface(ctx, window);
        createVkPhysicalDevice(ctx, -1);
        createVkDevice(ctx);
        createSwapchain(ctx, window, 0);
        auto maxFif = ctx->maxFrameInFlight;
        ctx->descriptorSetMgr = new DescriptorSetManager(maxFif);
        ctx->cmdBufferMgr = new CommandBufferManager(maxFif);
        ctx->destroyer = new DeferredDestroyer(maxFif);

        auto& fences = ctx->mFences;
        fences.resize(ctx->maxFrameInFlight);

        for (auto&& fence : fences) {
            fence = createRsFence(ctx);
            resetRsFence(ctx, fence);
        }
        ctx->currentSwapchainImage = ctx->maxSwapChainImages;
        return ctx;
    }

    void deinitVulkanBackEnd(rs_context_vk* ctx, Window::rs_window* window)
    {
        if (ctx->computeQueue)
            vkQueueWaitIdle(ctx->computeQueue->queue);
        if(ctx->transferQueue)
            vkQueueWaitIdle(ctx->transferQueue->queue);

        vkQueueWaitIdle(ctx->graphicQueue->queue);

        if(ctx->presentQueue)
            vkQueueWaitIdle(ctx->presentQueue->queue);
        ctx->destroyer->clearAll(ctx);
        destroyDevice(ctx);
        destroyVkInstance(ctx);
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

    rs_rendertarget_vk* createRsRenderTarget(rs_context_vk* ctx,const std::vector<rs_image_vk*>& images, rs_image_vk* depthStencil)
    {
        int iw = images[0]->width;
        int ih = images[0]->height;
        int il = images[0]->arrayLayers;
        VkFramebufferCreateInfo ci{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        std::vector<rs_image*> localimages;

        int depthAttPos = -1;

        for (int i = 0; i < images.size(); ++i) {
            auto& img = *(images[i]);
            if (!(img.usage & ImageUsage_ColorAttachment)) {
                assert(0 && "Image usage not match");
                return nullptr;
            }
            if (img.width != iw || img.height != ih || img.arrayLayers != il) {
                assert(0 && "Attachment size not match");
                return nullptr;
            }
            localimages.push_back((rs_image*) & img);
        }

        rs_rendertarget_vk* rt = new rs_rendertarget_vk;
        rt->m_attachments = localimages;
        rt->m_depthStencilAttachment = depthStencil;
        return rt;
    }

    void destroyRsRenderTarget(rs_context_vk* ctx, rs_rendertarget_vk*& rt)
    {
        delete rt;
        rt = 0;
    }

    VkCompareOp
        toVkCompareOp(CompareOp op)
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
        //TODO:
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
        ret->queueType = context->graphicQueue->queueType;
        return ret;
    }

    void destroyRsBuffer(rs_context_vk* context, rs_buffer_vk*& buffer, bool immediately)
    {
        if (!immediately) {
            context->destroyer->destroyBuffer(context->nextRenderFrame, buffer);
            return;
        }
        vmaDestroyBuffer(context->allocator, (VkBuffer)buffer->native, buffer->allocation);
        buffer->native = 0;
        delete buffer;
        buffer = 0;
    }

    void* mapRsBuffer(rs_context_vk* context, rs_buffer_vk* buffer)
    {
        assert(isRsBufferMappable(context, buffer));
        if (buffer->mappedPtr) {
            return buffer->mappedPtr;
        }
        void* mappedData = nullptr;
        VkResult result = vmaMapMemory(context->allocator, buffer->allocation, &mappedData);
        if (result != VK_SUCCESS) {
            return nullptr;
        }
        buffer->mappedPtr = mappedData;
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
        ret->usage = desc.usage;
        ret->sampleCount = desc.samples;
        return ret;
    }

    void destroyRsImage(rs_context_vk* context, rs_image_vk*& image, bool immediately)
    {
        if (!immediately) {
            context->destroyer->destroyImage(context->nextRenderFrame, image);
            return;
        }
        vmaDestroyImage(context->allocator, (VkImage)image->native, image->allocation);
        delete image;
        image = 0;
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

    void destroyRsSampler(rs_context_vk* context, rs_sampler_vk*& sampler, bool immediately)
    {
        if (!immediately) {
            context->destroyer->destroySampler(context->nextRenderFrame, sampler);
            return;
        }
        vkDestroySampler(context->device, (VkSampler)sampler->native,0);
        delete sampler;
        sampler = 0;
    }

    rs_shader_module_vk* createRsShader(rs_context_vk* context, ShaderDesc& desc)
    {
        VkShaderModuleCreateInfo ci{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        if (desc.isSpirv) {
            ci.codeSize = desc.codeSizeByte;
            ci.pCode = reinterpret_cast<const uint32_t*>(desc.shaderCode);
        }
        else {
            if (desc.compileDesc) {
                return compileShader(context, *desc.compileDesc);
            }
            else {
                return nullptr;
            }
        }
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

    void destroyRsShader(rs_context_vk* context, rs_shader_module_vk*& shaderModule)
    {
        vkDestroyShaderModule(context->device, (VkShaderModule)shaderModule->native, 0);
        delete shaderModule;
        shaderModule = 0;
    }

    rs_semaphore_vk* createRsSemaphore(rs_context_vk* ctx)
    {
        VkSemaphoreCreateInfo ci{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        rs_semaphore_vk* sem = new rs_semaphore_vk;
        vkCreateSemaphore(ctx->device, &ci, 0, (VkSemaphore*)( & sem->native));
        return sem;
    }

    void destroyRsSemaphore(rs_context_vk* ctx, rs_semaphore_vk*& sem)
    {
        vkDestroySemaphore(ctx->device, (VkSemaphore)sem->native, 0);
        delete sem;
        sem = 0;
    }

    rs_fence_vk* createRsFence(rs_context_vk* ctx)
    {
        VkFenceCreateInfo ci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        rs_fence_vk* fence = new rs_fence_vk;

        vkCreateFence(ctx->device, &ci, 0, (VkFence*)&fence->native);

        return fence;
    }

    void destroyRsFence(rs_context_vk* ctx, rs_fence_vk*& fence)
    {
        vkDestroyFence(ctx->device, (VkFence)fence->native, 0);
        delete fence;
        fence = 0;
    }

    void resetRsFence(rs_context_vk* ctx, rs_fence_vk* fence)
    {
        vkResetFences(ctx->device, 1, (VkFence*)&fence->native);
    }

    void waitForRsFence(rs_context_vk* ctx, rs_fence_vk* fence, uint64_t timeout)
    {
        vkWaitForFences(ctx->device, 1, (VkFence*)&fence->native, VK_TRUE, timeout);
    }

    rs_commandbuffer_vk* createRsCommand(rs_context_vk* ctx, const CommandBufferDesc& desc)
    {
        auto cmdMgr = ctx->cmdBufferMgr;
        return cmdMgr->getCmdBufferLocalThread(ctx, ctx->nextRenderFrame, desc.queueType);
    }

    void createSwapchain(rs_context_vk* context, ::Render::Window::rs_window* window, rs_swapchain* oldSwapchain)
    {
        using namespace Render::Window;
        rs_window_glfw* rsWindowGlfw = (rs_window_glfw*)window;
        GLFWwindow* glfwWin = (GLFWwindow*)rsWindowGlfw->nativeHandle();
        //Need vulkan extension --> khr xxxxxxx    
        VkSurfaceKHR surface = context->swapchain->surface;
        VkExtent2D extent;
        int wHeight, wWidth;
        window->getFramebufferSize(wWidth, wHeight);
        extent.width = wWidth;
        extent.height = wHeight;
        auto swapchainFormats = querySwapchainFormats(context->physicalDevice, surface);
        auto presentModes = querySwapchainPresentModes(context->physicalDevice, surface);
        VkSurfaceCapabilitiesKHR physicalCap;
        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physicalDevice, surface, &physicalCap), {
            uint64_t _ = 0;
            (void)*((int*)_);
        });
        auto choosenFormat = chooseSwapSurfaceFormat(swapchainFormats);
        auto choosePresentMode = chooseSwapPresentMode(presentModes);
        auto chooseImageCount = chooseSwapchainImageCount(physicalCap);

        VkSwapchainCreateInfoKHR sci{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        sci.surface = surface;
        sci.minImageCount = chooseImageCount;
        sci.imageFormat = choosenFormat.format;
        sci.imageColorSpace = choosenFormat.colorSpace;
        sci.imageExtent = extent;
        sci.imageArrayLayers = 1;
        sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        // 假设 graphicsQueueFamily 和 presentQueueFamily 相同
        sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        sci.preTransform = physicalCap.currentTransform;
        sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        sci.presentMode = choosePresentMode;
        sci.clipped = VK_TRUE;
        if (oldSwapchain)
        {
            sci.oldSwapchain = (VkSwapchainKHR)oldSwapchain->native;
        }
        VkSwapchainKHR swapchain;
        VK_CHECK(vkCreateSwapchainKHR(context->device, &sci, nullptr, &swapchain), {
            uint64_t _ = 0;
            (void)*((int*)_);
        });

        uint32_t swapImgCount = 0;
        vkGetSwapchainImagesKHR(context->device, swapchain, &swapImgCount, nullptr);
        std::vector<VkImage> swapImages(swapImgCount);
        vkGetSwapchainImagesKHR(context->device, swapchain, &swapImgCount, swapImages.data());
        std::vector<rs_image_vk*>swapchainImages;
        for (auto&& img : swapImages) {

            VkImageViewCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            ci.                    image  = img;
            ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            ci.                   format = choosenFormat.format;
            ci.components = { 
                VK_COMPONENT_SWIZZLE_IDENTITY ,
                VK_COMPONENT_SWIZZLE_IDENTITY ,
                VK_COMPONENT_SWIZZLE_IDENTITY ,
                VK_COMPONENT_SWIZZLE_IDENTITY 
            };
            auto& subres = ci.subresourceRange;
            subres.    aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subres.baseMipLevel = 0;
            subres.levelCount = 1;
            subres.baseArrayLayer = 0;
            subres.layerCount = 1;
            VkImageView view;
            VK_CHECK(vkCreateImageView(context->device, &ci, 0, &view), {
                swapchainImages.push_back(nullptr);
            });
            rs_image_vk* rsImage = new rs_image_vk;
            rsImage->native = img;
            rsImage->view = view;
            rsImage->width = wWidth;
            rsImage->height = wHeight;
            rsImage->type = ImageType::V2D;
            rsImage->depth = 1;
            rsImage->mipLevels = 1;
            rsImage->usage = ImageUsage_PresentSrc | ImageUsage_ColorAttachment;
            rsImage->format = ToImageFormat(choosenFormat.format);
            rsImage->arrayLayers = 1;
            swapchainImages.push_back(rsImage);
        }

        context->swapchain->native = swapchain;
        context->swapchain->swapchainImgs = swapchainImages;
        context->maxSwapChainImages = swapchainImages.size();
    }

    void destroySwapChain(rs_context_vk* context)
    {
        VkSwapchainKHR swapchain = (VkSwapchainKHR)context->swapchain->native;
        vkDestroySwapchainKHR(context->device,swapchain,0);
    }

    void createSurface(rs_context_vk* context, ::Render::Window::rs_window* window)
    {
        if (!context->swapchain) {
            context->swapchain = new rs_swapchain_vk;
        }
        VK_CHECK(glfwCreateWindowSurface(context->instance, (GLFWwindow*)window->nativeHandle(), 0, &context->swapchain->surface), { std::abort(); )};
    }

    int rateDeviceSuitability(rs_context_vk* context,VkPhysicalDevice device) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(device, &props);
        VkPhysicalDeviceFeatures feats;
        vkGetPhysicalDeviceFeatures(device, &feats);

        int score = 0;

        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }

        score += props.limits.maxImageDimension2D;
        score += props.limits.maxFramebufferHeight;
        score += props.limits.maxVertexInputAttributes;
        return score;
    }


    void createVkInstance(rs_context_vk* context)
    {
        volkInitialize();
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = context->initDesc.appName.c_str();
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = context->initDesc.engineName.c_str();
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3; // 或 VK_API_VERSION_1_0

        auto extension = getExtensionEnableInstance(context);
        auto layers = getLayerEnableInstance(context);

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.                    enabledLayerCount = layers.size();
        createInfo.ppEnabledLayerNames = layers.data();
        createInfo.                    enabledExtensionCount = extension.size();
        createInfo.ppEnabledExtensionNames = extension.data();
        VK_CHECK(vkCreateInstance(&createInfo, 0, &context->instance), { std::abort(); });
        volkLoadInstance(context->instance);
        if (context->initDesc.enableValidation) {
            createDebugUtilsMessengerEXT(context);
        }
    }

    void destroyVkInstance(rs_context_vk* context)
    {
        destroyDebugUtilsMessengerEXT(context);
        vkDestroyInstance(context->instance, 0);
        context->instance = 0;
        volkFinalize();
    }

    void createVkPhysicalDevice(rs_context_vk* context, int chooseOne)
    {
        uint32_t physicalDeviceCount = 0;
        VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physicalDeviceCount, nullptr), { std::abort(); });
        
        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        vkEnumeratePhysicalDevices(context->instance, &physicalDeviceCount, physicalDevices.data());


        if (chooseOne < 0 || chooseOne >= physicalDeviceCount) {
            std::vector<std::string> deviceInfos;
            deviceInfos.reserve(physicalDeviceCount);

            int maxScore = -1;
            int suitableDevice = 0;
            int it = 0;
            for (auto device : physicalDevices) {
                VkPhysicalDeviceProperties props;
                vkGetPhysicalDeviceProperties(device, &props);

                // 注意：driverVersion 的编码厂商可能不同，这里采用 VK_VERSION_* 宏来拆解
                uint32_t drvVer = props.driverVersion;
                uint32_t drvMajor = VK_VERSION_MAJOR(drvVer);
                uint32_t drvMinor = VK_VERSION_MINOR(drvVer);
                uint32_t drvPatch = VK_VERSION_PATCH(drvVer);

                // 拼接字符串："<Name> (Driver: x.y.z)"
                std::string info = std::string(props.deviceName)
                    + " (Driver: "
                    + std::to_string(drvMajor) + "."
                    + std::to_string(drvMinor) + "."
                    + std::to_string(drvPatch) + ")";

                deviceInfos.push_back(std::move(info));
                int curScore = rateDeviceSuitability(context, device);
                if (curScore > maxScore) {
                    maxScore = curScore;
                    suitableDevice = it;
                }
                ++it;
            }
            context->physicalDevice = physicalDevices[suitableDevice];
            context->physicalDevices = std::move(deviceInfos);
            
        }
        else {
            context->physicalDevice = physicalDevices[chooseOne];

        }
    }

    // 1) 查找只需支持 GRAPHICS 的队列族
    static bool findGraphicsFamily(VkPhysicalDevice phys,
        uint32_t& outFamily)
    {
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(phys, &count, nullptr);
        if (count == 0) return false;

        std::vector<VkQueueFamilyProperties> props(count);
        vkGetPhysicalDeviceQueueFamilyProperties(phys, &count, props.data());

        for (uint32_t i = 0; i < count; ++i) {
            if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                outFamily = i;
                return true;
            }
        }
        return false;
    }

    // 2) 查找只需支持 PRESENT 的队列族
    static bool findPresentFamily(VkPhysicalDevice phys,
        VkSurfaceKHR   surface,
        uint32_t& outFamily)
    {
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(phys, &count, nullptr);
        if (count == 0) return false;

        std::vector<VkQueueFamilyProperties> props(count);
        vkGetPhysicalDeviceQueueFamilyProperties(phys, &count, props.data());

        for (uint32_t i = 0; i < count; ++i) {
            VkBool32 present = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(phys, i, surface, &present);
            if (present) {
                outFamily = i;
                return true;
            }
        }
        return false;
    }


    static bool findComputeFamily(VkPhysicalDevice phys,
        bool           separate,
        uint32_t       graphicsFamily,
        uint32_t& outFamily)
    {
        if (!separate) {
            outFamily = graphicsFamily;
            return true;
        }

        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(phys, &count, nullptr);
        if (count == 0) return false;

        std::vector<VkQueueFamilyProperties> props(count);
        vkGetPhysicalDeviceQueueFamilyProperties(phys, &count, props.data());

        // 1) 优先找只支持 COMPUTE 的
        for (uint32_t i = 0; i < count; ++i) {
            bool onlyCompute = (props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
                !(props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT);
            if (onlyCompute) {
                outFamily = i;
                return true;
            }
        }
        // 2) 回退：找任意支持 COMPUTE 的
        for (uint32_t i = 0; i < count; ++i) {
            if (props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                outFamily = i;
                return true;
            }
        }
        return false;
    }

    // 查找 Transfer 队列族；如果 separate==false，则直接返回 graphicsFamily
    static bool findTransferFamily(VkPhysicalDevice phys,
        bool           separate,
        uint32_t       graphicsFamily,
        uint32_t& outFamily)
    {
        if (!separate) {
            outFamily = graphicsFamily;
            return true;
        }

        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(phys, &count, nullptr);
        if (count == 0) return false;

        std::vector<VkQueueFamilyProperties> props(count);
        vkGetPhysicalDeviceQueueFamilyProperties(phys, &count, props.data());

        // 1) 优先找只支持 TRANSFER 的
        for (uint32_t i = 0; i < count; ++i) {
            bool onlyTransfer = (props[i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                !(props[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT));
            if (onlyTransfer) {
                outFamily = i;
                return true;
            }
        }
        // 2) 回退：找任意支持 TRANSFER 的
        for (uint32_t i = 0; i < count; ++i) {
            if (props[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
                outFamily = i;
                return true;
            }
        }
        return false;
    }
    void createVkQueue(rs_context_vk* context, bool seperateCompute, bool seperateTransfer)
    {
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(context->physicalDevice, &count, nullptr);
        std::vector<VkQueueFamilyProperties> props(count);
        vkGetPhysicalDeviceQueueFamilyProperties(context->physicalDevice, &count, props.data());

        rs_queue_vk* graphic = new rs_queue_vk;
        graphic->queueType = QueueType_Graphics;
        if (!findGraphicsFamily(context->physicalDevice, graphic->familyIndex)) {
            std::abort();
        }

        rs_queue_vk* present = new rs_queue_vk;
        if (!findPresentFamily(context->physicalDevice, context->swapchain->surface, present->familyIndex)) {
            std::abort();
        }
        present->queueType = QueueType_Present;

        rs_queue_vk* compute = new rs_queue_vk;
        bool haveSeperateCompute = findComputeFamily(context->physicalDevice, seperateCompute, graphic->familyIndex, compute->familyIndex);
        if (!haveSeperateCompute || !seperateCompute) {
            delete compute;
            compute = 0;
            graphic->queueType |= QueueType_Compute;
        }

        rs_queue_vk* transfer = new rs_queue_vk;
        bool haveSeperateTranser = findTransferFamily(context->physicalDevice, seperateTransfer, graphic->familyIndex, transfer->familyIndex);
        if (!haveSeperateCompute || !seperateTransfer) {
            delete transfer;
            transfer = 0;
            graphic->queueType |= QueueType_Transfer;
        }

        context->graphicQueue = graphic;
        context->presentQueue = present;
        context->computeQueue = compute;
        context->transferQueue = transfer;
    }

    void destroyVkQueue(rs_context_vk* context)
    {
        auto& ctx = context;
        delete ctx->graphicQueue;
          ctx->graphicQueue = 0;
        delete ctx->computeQueue;
          ctx->computeQueue = 0;
        delete ctx->transferQueue;
         ctx->transferQueue = 0;
        delete ctx->presentQueue; 
         ctx->presentQueue = 0; 
    }

    void createVkDevice(rs_context_vk* context)
    {
        createVkQueue(context, context->initDesc.asyncTransferCompute, context->initDesc.asyncTransferCompute);
        auto extensionRequired = getExtensionEnableDevice(context);
        auto deviceFeatureEnable = getExtensionEnablePhysicalDevice(context);
        VkDeviceCreateInfo createInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        float unifiedPriorities = 1.0f;
        int graphicQueueCount = 1;
        std::vector< VkDeviceQueueCreateInfo> queueInfos;
        bool seperateCompute = false;
        bool seperatePresent = false;
        bool seperateTransfer = false;
        {
            VkDeviceQueueCreateInfo ci{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
            ci.queueFamilyIndex = context->graphicQueue->familyIndex;
            ci.queueCount = 1;
            ci.pQueuePriorities = &unifiedPriorities;
            queueInfos.push_back(ci);
        }

        if (context->transferQueue && context->transferQueue->familyIndex != context->graphicQueue->familyIndex) {
            seperateTransfer = true;
            VkDeviceQueueCreateInfo ci{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
            ci.                    queueFamilyIndex = context->transferQueue->familyIndex;
            ci.                    queueCount = 1;
            ci.pQueuePriorities = &unifiedPriorities;
            queueInfos.push_back(ci);
        }
        if (context->computeQueue && context->computeQueue->familyIndex != context->graphicQueue->familyIndex) {
            seperateCompute = true;
            VkDeviceQueueCreateInfo ci{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
            ci.queueFamilyIndex = context->computeQueue->familyIndex;
            ci.queueCount = 1;
            ci.pQueuePriorities = &unifiedPriorities;
            queueInfos.push_back(ci);
        }

        if (context->presentQueue && context->presentQueue->familyIndex != context->graphicQueue->familyIndex) {
            seperatePresent = true;
            VkDeviceQueueCreateInfo ci{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
            ci.queueFamilyIndex = context->presentQueue->familyIndex;
            ci.queueCount = 1;
            ci.pQueuePriorities = &unifiedPriorities;
            queueInfos.push_back(ci);
        }



        createInfo.                         queueCreateInfoCount = queueInfos.size();
        createInfo.pQueueCreateInfos = queueInfos.data();
        createInfo.enabledLayerCount = 0;
        createInfo.                           enabledExtensionCount = extensionRequired.size();
        createInfo.ppEnabledExtensionNames = extensionRequired.data();
        createInfo. pEnabledFeatures = &deviceFeatureEnable;
        
        vkCreateDevice(context->physicalDevice, &createInfo, 0, &context->device);
        vkGetDeviceQueue(context->device, context->graphicQueue->familyIndex, 0, &context->graphicQueue->queue);
        if (context->computeQueue)
        {
            vkGetDeviceQueue(context->device, context->computeQueue->familyIndex, 0, &context->computeQueue->queue);
        }
        if (context->transferQueue) {
            vkGetDeviceQueue(context->device, context->transferQueue->familyIndex, 0, &context->transferQueue->queue);
        }
        if (context->presentQueue)
        {
            vkGetDeviceQueue(context->device, context->presentQueue->familyIndex, 0, &context->presentQueue->queue);
        }
        volkLoadDevice(context->device);
    }

    void destroyDevice(rs_context_vk* context)
    {
        vkDestroyDevice(context->device,0);
        context->device = 0;
    }

    void createDebugUtilsMessengerEXT(rs_context_vk* ctx)
    {
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugCallback;
        debugCreateInfo.pUserData = nullptr;

        if (ctx->initDesc.enableValidation) {
            if (vkCreateDebugUtilsMessengerEXT(ctx->instance, &debugCreateInfo, nullptr, &ctx->validationObject)
                != VK_SUCCESS) {
                return;
            }
        }
    }

    void destroyDebugUtilsMessengerEXT(rs_context_vk* ctx)
    {
        if (!ctx->validationObject)return;
        vkDestroyDebugUtilsMessengerEXT(ctx->instance, ctx->validationObject, 0);
        ctx->validationObject = 0;
    }

    std::vector<const char*> getExtensionEnableDevice(rs_context_vk* context)
    {
        uint32_t extCount = 0;
        vkEnumerateDeviceExtensionProperties(context->physicalDevice,nullptr, &extCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extCount);
        vkEnumerateDeviceExtensionProperties(context->physicalDevice, nullptr, &extCount, availableExtensions.data());
        std::set<std::string> extensionSet;
        for (auto&& i : availableExtensions) {
            extensionSet.insert(std::string(i.extensionName));
        }

        std::vector<const char*> requiredExtensionNames{};
            
        //swapchain extension
        requiredExtensionNames.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        //Remove unsuppored extension
        auto itor = std::remove_if(requiredExtensionNames.begin(), requiredExtensionNames.end(), [&extensionSet](const char* in) {
            auto itor = extensionSet.find(std::string(in));
            if (itor == extensionSet.end())return true;
            return false;
            });

        requiredExtensionNames.erase(itor, requiredExtensionNames.end());
        return requiredExtensionNames;
    }

    VkPhysicalDeviceFeatures getExtensionEnablePhysicalDevice(rs_context_vk* context)
    {
        VkPhysicalDeviceFeatures referenceFeature;
        vkGetPhysicalDeviceFeatures(context->physicalDevice, &referenceFeature);
        return referenceFeature;
    }

    VkPhysicalDeviceFeatures2 getExtensionEnablePhysicalDevice2(rs_context_vk* context)
    {
        VkPhysicalDeviceFeatures2 referenceFeature;
        vkGetPhysicalDeviceFeatures2(context->physicalDevice, &referenceFeature);
        return referenceFeature;
    }

    std::vector<const char*> getExtensionEnableInstance(rs_context_vk* context)
    {
        uint32_t extCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr ,&extCount,nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extCount, availableExtensions.data());
        std::set<std::string> extensionSet;
        for (auto&& i : availableExtensions) {
            extensionSet.insert(std::string(i.extensionName));
        }

        std::vector<const char*> requiredExtensionNames{};

        uint32_t glfwrequired;
        const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwrequired);
        for (auto i = 0; i < glfwrequired; ++i) {
            requiredExtensionNames.push_back(glfwExts[i]);
        }
        
        if (context->initDesc.enableValidation) {
            requiredExtensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        //Remove unsuppored extension
        auto itor = std::remove_if(requiredExtensionNames.begin(), requiredExtensionNames.end(), [&extensionSet](const char* in) {
            auto itor = extensionSet.find(std::string(in));
            if (itor == extensionSet.end())return true;
            return false;
        });

        requiredExtensionNames.erase(itor, requiredExtensionNames.end());
        return requiredExtensionNames;
    }

    std::vector<const char*> getLayerEnableInstance(rs_context_vk* context)
    {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties( &layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        std::set<std::string> layerSet;
        for (auto&& i : availableLayers) {
            layerSet.insert(std::string(i.layerName));
        }

        std::vector<const char*> requiredLayerNames{};

        if (context->initDesc.enableValidation) {
            requiredLayerNames.push_back("VK_LAYER_KHRONOS_validation");
        }

        //Remove unsuppored extension
        auto itor = std::remove_if(requiredLayerNames.begin(), requiredLayerNames.end(), [&layerSet](const char* in) {
            auto itor = layerSet.find(std::string(in));
            if (itor == layerSet.end())return true;
            return false;
            });

        requiredLayerNames.erase(itor, requiredLayerNames.end());
        return requiredLayerNames;
    }

    rs_buffer_vk* createStageBufferTemp(rs_context_vk* context, uint64_t size)
    {
        BufferDesc stageBufferDesc{};
        stageBufferDesc.bufUsage = BufferType_TransferSrc;
        stageBufferDesc.mappable = true;
        stageBufferDesc.queueType = QueueType_Graphics;
        stageBufferDesc.byteSize = size;
        return createRsBuffer(context, stageBufferDesc);
    }

    void cmdBeginRenderPass(rs_commandbuffer_vk* cb, rs_renderpass_vk* renderpass, std::vector<ClearColor>& clearColor, ClearDepthStencil& clearDs)
    {
        std::vector< VkClearValue> clearValues;
        clearValues.reserve(renderpass->passDesc.attachments.size());
        
        //The last desc is depthAndStencil
        for (int i = 0; i < clearColor.size() - 1; ++i) {
            auto& c = clearColor[i];
            VkClearValue clr{};
            auto& passDesc = renderpass->passDesc.attachments[i];
            if (passDesc.isHDR) {
                clr.color.float32[0] = c.rgba[0];
                clr.color.float32[1] = c.rgba[1];
                clr.color.float32[2] = c.rgba[2];
                clr.color.float32[3] = c.rgba[3];
            }
            else {
                clr.color.uint32[0] = (uint32_t)(c.rgba[0] * 255.f);
                clr.color.uint32[1] = (uint32_t)(c.rgba[1] * 255.f);
                clr.color.uint32[2] = (uint32_t)(c.rgba[2] * 255.f);
                clr.color.uint32[3] = (uint32_t)(c.rgba[3] * 255.f);
            }
            clearValues.push_back(clr);
        }

        if (renderpass->haveDepth) {
            VkClearValue clr{};
            clr.depthStencil.depth = clearDs.depth;
            clr.depthStencil.stencil = clearDs.stencil;
            clearValues.push_back(clr);
        }

        VkRenderPassBeginInfo info{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        info.renderPass = (VkRenderPass)renderpass->native;
        info.framebuffer = renderpass->frameBuffer;

        auto& extent = info.renderArea.extent;
        extent.width = renderpass->width;
        extent.width = renderpass->height;
        info.               clearValueCount = clearValues.size();
        info.               pClearValues = clearValues.data();

        vkCmdBeginRenderPass((VkCommandBuffer)cb->native, &info, VK_SUBPASS_CONTENTS_INLINE);
        cb->currentRenderPass = renderpass;
    }

    void cmdEndRenderPass(rs_commandbuffer_vk* cb)
    {
        cb->currentRenderPass = 0;
        vkCmdEndRenderPass((VkCommandBuffer)cb->native);
    }

    void cmdSetViewport(rs_commandbuffer_vk* cb, Rect2D& rect)
    {
        assert(cb->currentRenderPass != nullptr);
        VkViewport viewport{};
        viewport.x = rect.l * cb->currentRenderPass->width;
        viewport.y = rect.t * cb->currentRenderPass->height;
        viewport.width = (rect.r - rect.l) * cb->currentRenderPass->width;
        viewport.height = (rect.b - rect.t) * cb->currentRenderPass->height;
        vkCmdSetViewport((VkCommandBuffer)cb->native, 0, 1, &viewport);
    }

    void cmdSetScissor(rs_commandbuffer_vk* cb, Rect2D& rect)
    {
        assert(cb->currentRenderPass != nullptr);
        VkRect2D scissor{};
        scissor.extent.width = (rect.r - rect.l) * cb->currentRenderPass->width;
        scissor.extent.height = (rect.b - rect.t) * cb->currentRenderPass->height;
        scissor.offset.x = rect.l * cb->currentRenderPass->width;
        scissor.offset.y = rect.t * cb->currentRenderPass->height;

        vkCmdSetScissor((VkCommandBuffer)cb->native, 0, 1, &scissor);
    }

    void cmdDrawIndexed(rs_commandbuffer_vk* cb, const RenderInfo& info, bool isInstanced)
    {
        if (info.pipeline == 0) {
            assert(0);
            return;
        }
        rs_pipeline_vk* pipeline = (rs_pipeline_vk*)info.pipeline;
        auto& bufferBinding = pipeline->vtxInput.bindings;
        if (bufferBinding.size() != info.bindingBuffers.size()) {
            assert(0);
            return;
        }
        vkCmdBindPipeline((VkCommandBuffer)cb->native, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipeline)info.pipeline->native);

        vkCmdBindIndexBuffer((VkCommandBuffer)(cb->native), (VkBuffer)info.indexBuffer, info.idxOffset, info.indexType == IndexType::Uint16 ? VkIndexType::VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
        std::vector<VkBuffer> bindingBuffers;
        std::vector<VkDeviceSize> bufferoffsets;
        for (int i = 0; i < info.bindingBuffers.size(); ++i) {
            bindingBuffers.push_back((VkBuffer)(info.bindingBuffers[i].buffer->native));
            bufferoffsets.push_back(info.bindingBuffers[i].offset);
        }
        vkCmdBindVertexBuffers((VkCommandBuffer)cb->native, 0, bufferBinding.size(), bindingBuffers.data(), bufferoffsets.data());
        VkPipelineLayout pLayut = (VkPipelineLayout)(((rs_pipeline_vk*)info.pipeline)->layout->native);
        for (const auto& [setIdx, bindingData] : info.descriptors) {
            auto descriptorSet = bindingData;
            auto descriptorSetVk = (VkDescriptorSet)descriptorSet->native;
            
            vkCmdBindDescriptorSets((VkCommandBuffer)cb->native, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, pLayut, setIdx, 1, &descriptorSetVk, 0, 0);
        }
        uint32_t instanceCnt = isInstanced ? info.instanceCount : 1;

        vkCmdDrawIndexed((VkCommandBuffer)cb->native, info.idxCount, instanceCnt, 0, info.vtxoffset, 0);
    }

    uint32_t bufferBarrierTransferDstCalc(rs_buffer_vk* buffer) {
        auto bufferType = buffer->bufferType;
        uint32_t dstAccessBits = 0;
        if (bufferType & BufferType_Vertex) {
            dstAccessBits |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        }
        if (bufferType & BufferType_Index) {
            dstAccessBits |= VK_ACCESS_INDEX_READ_BIT;
        }
        if (bufferType & BufferType_Uniform) {
            dstAccessBits |= VK_ACCESS_UNIFORM_READ_BIT;
        }
        if (bufferType & BufferType_Storage) {
            dstAccessBits |= VK_ACCESS_SHADER_READ_BIT;
        }
        if (bufferType & BufferType_TransferSrc) {
            dstAccessBits |= VK_ACCESS_HOST_READ_BIT& VK_ACCESS_TRANSFER_READ_BIT & VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        if (bufferType & BufferType_Indirect) {
            dstAccessBits |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        }
        return dstAccessBits;
    }

    void cmdUpdateBufferData(rs_commandbuffer_vk* cb, rs_context_vk* ctx, rs_buffer_vk* buffer, void* data, uint64_t size,uint64_t dstOffset)
    {
        assert(buffer && data);
        uint64_t cpSize = std::min((uint64_t)buffer->byteSize, size);
        if (buffer->mappedPtr) {
            memcpy(buffer->mappedPtr, data, size);
            return;
        }

        auto tempBuffer = createStageBufferTemp(ctx, size);
        auto descPtr = mapRsBuffer(ctx, tempBuffer);
        memcpy(descPtr, data, size);
        VkCopyBufferInfo2 cpInfo{
            VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2
        };

        cpInfo.srcBuffer = (VkBuffer)tempBuffer->native;
        cpInfo.dstBuffer = (VkBuffer)buffer->native;
        cpInfo.regionCount = 1;
        VkBufferCopy2 bufferCopy{
            VK_STRUCTURE_TYPE_BUFFER_COPY_2
        };
        cpInfo.pRegions = &bufferCopy;

        bufferCopy.        srcOffset = 0;
        bufferCopy.        dstOffset = dstOffset;
        bufferCopy.        size = size;

        VkBufferMemoryBarrier barrierBefore = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT & VK_ACCESS_TRANSFER_READ_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = (VkBuffer)tempBuffer->native,
        .offset = 0,
        .size = VK_WHOLE_SIZE,
        };

        VkBufferMemoryBarrier barrierAfter = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = bufferBarrierTransferDstCalc(buffer),
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = (VkBuffer)buffer->native,
        .offset = dstOffset,
        .size = VK_WHOLE_SIZE,
        };

        auto cmd = (VkCommandBuffer)cb->native;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 1, &barrierBefore, 0, 0);
        vkCmdCopyBuffer2(cmd,&cpInfo);
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, 0, 1, &barrierAfter, 0, 0);
        destroyRsBuffer(ctx,tempBuffer);

    }


    VkAccessFlags calcSrcImageLayoutAccessStage(VkImageLayout oldLayout) {
        switch (oldLayout) {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            return 0;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            return VK_ACCESS_HOST_WRITE_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return VK_ACCESS_TRANSFER_READ_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            return VK_ACCESS_TRANSFER_WRITE_BIT;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return VK_ACCESS_SHADER_READ_BIT;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            return VK_ACCESS_MEMORY_READ_BIT;
        default:
            // Fallback to all read/write
            return VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        }
    }

    // Utility: map VkImageLayout (newLayout) to appropriate dstAccessMask for pipeline barriers
    VkAccessFlags calcDstImageLayoutAccessStage(VkImageLayout newLayout) {
        switch (newLayout) {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            return VK_ACCESS_TRANSFER_WRITE_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return VK_ACCESS_TRANSFER_READ_BIT;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return VK_ACCESS_SHADER_READ_BIT;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            return VK_ACCESS_MEMORY_READ_BIT;
        default:
            return VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        }
    }

    VkPipelineStageFlags getSrcStageForLayout(VkImageLayout oldLayout) {
        switch (oldLayout) {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            return VK_PIPELINE_STAGE_HOST_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        default:
            // Fallback to all commands if unknown
            return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        }
    }

    VkPipelineStageFlags getDstStageForLayout(VkImageLayout newLayout) {
        switch (newLayout) {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        default:
            // Fallback to all commands if unknown
            return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        }
    }

    void cmdImageLayoutTo(rs_commandbuffer_vk* cb, rs_image_vk* image, VkImageLayout newlayout, uint32_t mip, uint32_t layer, uint32_t aspect)
    {
        VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = calcSrcImageLayoutAccessStage(image->currentLayout),  // 前一个布局带来的访问屏障
        .dstAccessMask = calcDstImageLayoutAccessStage(newlayout),
        .oldLayout = image->currentLayout,                             // 比如：VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        .newLayout = newlayout,  // 要切到 transfer dst
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = (VkImage)image->native,
        .subresourceRange = {
            .aspectMask = aspect,
            .baseMipLevel = mip,   // 只针对你要写入的 mip 级
            .levelCount = 1,
            .baseArrayLayer = layer,   // 只针对你要写入的那一层
            .layerCount = 1,
        },

        };
        vkCmdPipelineBarrier((VkCommandBuffer)cb->native, getSrcStageForLayout(image->currentLayout), getDstStageForLayout(newlayout), 0, 0, 0, 0, 0,1 , &barrier);
        image->currentLayout = newlayout;
    }

    void cmdSubmitCmdBuffer(rs_context_vk* ctx, rs_commandbuffer_vk* cb, QueueType queue, std::vector<rs_semaphore_vk*> waitSemaphores, std::vector<rs_semaphore_vk*> signalSemaphores, rs_fence_vk* fence)
    {
        std::vector<VkSemaphore> ntvwaitSemaphores,ntvsignalSemaphores;
        for (auto&& sem : waitSemaphores) {
            ntvwaitSemaphores.push_back((VkSemaphore)sem->native);
        }
        for (auto&& sem : signalSemaphores) {
            ntvsignalSemaphores.push_back((VkSemaphore)sem->native);
        }
        VkSubmitInfo info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };

        info.                       waitSemaphoreCount = ntvwaitSemaphores.size();
        info.                       pWaitSemaphores = ntvwaitSemaphores.data();
        info.commandBufferCount = 1;
        info.pCommandBuffers = (VkCommandBuffer*)&cb->native;
        info.signalSemaphoreCount = ntvsignalSemaphores.size();
        info.pSignalSemaphores = ntvsignalSemaphores.data();

        rs_queue_vk* rsQueue;

        switch (queue)
        {
        case Render::QueueType_Graphics:
            rsQueue = ctx->graphicQueue;
            break;
        case Render::QueueType_Compute:
            rsQueue = ctx->computeQueue;
            break;
        case Render::QueueType_Transfer:
            rsQueue = ctx->transferQueue;
            break;
        case Render::QueueType_Present:
            rsQueue = ctx->presentQueue;
            break;
        default:
            rsQueue = ctx->graphicQueue;
            break;
        }

        vkQueueSubmit(rsQueue->queue,1, &info, fence != nullptr ? (VkFence)fence->native : VK_NULL_HANDLE);
    }

    uint64_t beginRsFrameVk(rs_context_vk* ctx)
    {
        uint64_t renderFrame = ctx->nextRenderFrame;
        uint64_t newRenderFrame = renderFrame++;
        uint64_t maxFif = ctx->maxFrameInFlight;
        auto cmdbufMgr = ctx->cmdBufferMgr;
        auto descriptorSetMgr = ctx->descriptorSetMgr;

        //Wait newRenderFrame % maxFif finish
        uint64_t toWaitFrame = newRenderFrame % maxFif;

        waitForRsFence(ctx, ctx->mFences[toWaitFrame],10000000ull);

        cmdbufMgr->beginFrame(ctx,newRenderFrame);
        descriptorSetMgr->beginFrame(ctx, newRenderFrame);

        return 0;
    }

    uint64_t waitForNextPresentImage(rs_context_vk* ctx, rs_semaphore_vk* SemaphoreToSignal, rs_fence_vk* fenceToSignal)
    {
        uint32_t swapImageIdx;
        VkSemaphore sem = SemaphoreToSignal == 0 ? VK_NULL_HANDLE : (VkSemaphore)SemaphoreToSignal->native;
        VkFence fence = fenceToSignal == 0 ? VK_NULL_HANDLE : (VkFence)fenceToSignal->native;
        vkAcquireNextImageKHR(ctx->device, (VkSwapchainKHR)ctx->swapchain->native, 100000000,sem , fence, &swapImageIdx);
        return swapImageIdx;
    }

    void submitToPresentImage(rs_context_vk* ctx, uint32_t presentImgIdx, std::vector<rs_semaphore_vk*> semsToWait)
    {
        std::vector<VkSemaphore> semphoresToWait;
        for (auto&& semRs : semsToWait) {
            semphoresToWait.push_back((VkSemaphore)semRs->native);
        }
        VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount;
        presentInfo.pWaitSemaphores = semphoresToWait.data();
        presentInfo.swapchainCount = semphoresToWait.size();
        presentInfo.pSwapchains = (VkSwapchainKHR*)&ctx->swapchain->native;
        presentInfo.pImageIndices = &presentImgIdx;
        vkQueuePresentKHR(ctx->presentQueue->queue, &presentInfo);
    }

    void cmdUpdateImage(rs_commandbuffer_vk* cb, rs_context_vk* context, rs_image_vk* image, void* data, uint64_t size, uint32_t dstMip, uint32_t layer)
    {
        assert(image && data);
        auto cmd = (VkCommandBuffer)cb->native;
        auto tempBuffer = createStageBufferTemp(context, size);
        auto dstPtr = mapRsBuffer(context, tempBuffer);
        memcpy(dstPtr, data, size);


        VkImageSubresourceLayers    imageSubresource{};
        imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageSubresource.mipLevel = dstMip;
        imageSubresource.baseArrayLayer = layer;
        imageSubresource.layerCount = 1;
        VkOffset3D                  imageOffset{ 0,0,0 };
        VkExtent3D                  imageExtent{ image->width,image->height,image->depth };
        VkBufferImageCopy2 imageCpy{ VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2 };
        imageCpy.bufferOffset = 0;
        imageCpy.bufferRowLength = 0;
        imageCpy.bufferImageHeight = 0;
        imageCpy.imageSubresource = imageSubresource;
        imageCpy.imageOffset = imageOffset;
        imageCpy.imageExtent = imageExtent;

        VkBufferMemoryBarrier barrierBefore = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = (VkBuffer)tempBuffer->native,
        .offset = 0,
        .size = VK_WHOLE_SIZE,
        };

        VkBufferMemoryBarrier barrierAfter = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = calcDstImageLayoutAccessStage(image->currentLayout),
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = (VkBuffer)tempBuffer->native,
        .offset = 0,
        .size = VK_WHOLE_SIZE,
        };

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 1, &barrierBefore, 0, 0);
        auto layoutSave = image->currentLayout;
        cmdImageLayoutTo(cb, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstMip, layer, VK_IMAGE_ASPECT_COLOR_BIT);
        VkCopyBufferToImageInfo2 cpInfo{ VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2 };
        cpInfo.srcBuffer = (VkBuffer)tempBuffer->native;
        cpInfo.dstImage = (VkImage)image->native;
        cpInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        cpInfo.regionCount = 1;
        cpInfo.pRegions = &imageCpy;
        vkCmdCopyBufferToImage2(cmd, &cpInfo);
        cmdImageLayoutTo(cb, image, layoutSave, dstMip, layer, VK_IMAGE_ASPECT_COLOR_BIT);
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, getDstStageForLayout(layoutSave), 0, 0, 0, 1, &barrierAfter, 0, 0);
    }

}


