#define VK_NO_PROTOTYPES
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION
#include "volk.h"
#include "vk_mem_alloc.h"
#include "GLFW/glfw3.h"
#include "render_function.h"
#include "vulkan/vulkan_shader_reflect.h"
#include "window/render_resource_window_glfw.h"
#include "vulkan/vulkan_render_function.h"
#include "bit_helper.h"
#include "vulkan/vulkan_descriptor_set.h"
#include "vulkan/vulkan_command.h"
#include "vulkan/vulkan_deferred_destroy.h"
#include "vulkan/vulkan_pipeline.h"
#include "vulkan/vulkan_image_data.h"
#include "render_function.h"
#include "render_log.h"
#include <set>
#include <iostream>
namespace {
    inline const uint32_t VulkanVersion = VK_API_VERSION_1_3;

    void TransitionImageLayout(
        VkCommandBuffer commandBuffer,
        VkImage image,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        VkImageSubresourceRange subresourceRange,
        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
    ) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange = subresourceRange;

        // 默认 access mask
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;

        // 自动适配 layout 组合
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

        }
        else if (oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        }
        else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            barrier.dstAccessMask = 0;
            srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

        }
        else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        }
        else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

        }
        else {
            // fallback，用户可以手动指定stage
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = 0;
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            srcStage,
            dstStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    }

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

    void queryAllImageFormatCaps(rs_context_vk* ctx)
    {
        auto physDev = ctx->physicalDevice;

        for (int i = 0; i < int(ImageFormat::Invalid); ++i)
        {
            ImageFormat imgFmt = static_cast<ImageFormat>(i);
            VkFormat vkFmt = toVkFormat(imgFmt);

            FormatCapFlag caps = 0;

            // Invalid / Unknown 直接跳过
            if (vkFmt == VK_FORMAT_UNDEFINED)
            {
                ctx->ImageFormatCaps[i] = caps;
                continue;
            }

            VkFormatProperties props{};
            vkGetPhysicalDeviceFormatProperties(physDev, vkFmt, &props);

            VkFormatFeatureFlags features = props.optimalTilingFeatures;

            // 至少支持 optimal tiling 才算 Supported
            if (features != 0)
            {
                caps |= ImgFormatCaps::Supported;
            }

            // Color attachment
            if (features & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
            {
                caps |= ImgFormatCaps::ColorAtt;
            }
            if (features & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                caps |= ImgFormatCaps::DepthStencil;
            }
            // Shader sampling
            if (features & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
            {
                caps |= ImgFormatCaps::Sample;
            }

            // Storage image
            if (features & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)
            {
                caps |= ImgFormatCaps::Storage;
            }

            ctx->ImageFormatCaps[i] = caps;
        }
    }

    void initRenderTextureFormatMapping(rs_context_vk* ctx)
    {
        auto& map = ctx->rtFormatMap;
        for (auto& e : map)
        {
            e = ImageFormat::Invalid;
        }

        auto isColorSupported = [ctx](ImageFormat fmt) -> bool {
            return Render::queryImgFormatCaps(ctx, fmt, ImgFormatCaps::Supported | ImgFormatCaps::ColorAtt);
            };
        auto isDepthStencilSupported = [ctx](ImageFormat fmt) -> bool {
            return Render::queryImgFormatCaps(ctx, fmt, ImgFormatCaps::Supported | ImgFormatCaps::DepthStencil);
            };
        // ------------------------
        // LDR Color
        // ------------------------
        if (isColorSupported(ImageFormat::RGBA8_UNORM))
            map[(size_t)RenderTextureFormat::RGBA8] = ImageFormat::RGBA8_UNORM;
        else if (isColorSupported(ImageFormat::BGRA8_UNORM))
            map[(size_t)RenderTextureFormat::RGBA8] = ImageFormat::BGRA8_UNORM;

        // ------------------------
        // HDR Color
        // ------------------------
        if (isColorSupported(ImageFormat::R16_SFLOAT))
            map[(size_t)RenderTextureFormat::R16F] = ImageFormat::R16_SFLOAT;

        if (isColorSupported(ImageFormat::RG16_SFLOAT))
            map[(size_t)RenderTextureFormat::RG16F] = ImageFormat::RG16_SFLOAT;

        if (isColorSupported(ImageFormat::RGBA16_SFLOAT))
            map[(size_t)RenderTextureFormat::RGBA16F] = ImageFormat::RGBA16_SFLOAT;
        else if (isColorSupported(ImageFormat::RGBA32_SFLOAT))
            map[(size_t)RenderTextureFormat::RGBA16F] = ImageFormat::RGBA32_SFLOAT; // fallback
        
        if (isColorSupported(ImageFormat::RGBA32_SFLOAT))
            map[(size_t)RenderTextureFormat::RGBA32F] = ImageFormat::RGBA32_SFLOAT; // fallback
        else if (isColorSupported(ImageFormat::RGBA16_SFLOAT))
            map[(size_t)RenderTextureFormat::RGBA32F] = ImageFormat::RGBA16_SFLOAT; // fallback
        // ------------------------
        // Depth / Stencil
        // ------------------------
        if (isDepthStencilSupported(ImageFormat::D24_UNORM_S8_UINT))
            map[(size_t)RenderTextureFormat::D24S8] = ImageFormat::D24_UNORM_S8_UINT;
        else if (isDepthStencilSupported(ImageFormat::D32_SFLOAT_S8_UINT))
            map[(size_t)RenderTextureFormat::D24S8] = ImageFormat::D32_SFLOAT_S8_UINT;
        else if (isDepthStencilSupported(ImageFormat::D32_SFLOAT))
            map[(size_t)RenderTextureFormat::D24S8] = ImageFormat::D32_SFLOAT;

        map[(size_t)RenderTextureFormat::SwapchainFormat] = ctx->swapchain->SwapchainImageFormat;

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

    rs_context_vk* initVulkanBackEnd(const BackEndInitDesc& desc, Window::rs_window* window)
    {
        rs_context_vk* ctx = new rs_context_vk;
        ctx->initDesc = desc;
        createVkInstance(ctx);
        createSurface(ctx, window);
        createVkPhysicalDevice(ctx, -1);
        createVkDevice(ctx);
        createSwapchain(ctx, window, 0);

        queryAllImageFormatCaps(ctx);
        initRenderTextureFormatMapping(ctx);

        VmaVulkanFunctions vkFuncs{};
        vkFuncs.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vkFuncs.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
        VmaAllocatorCreateInfo vmaCi{};
        vmaCi.device = ctx->device;
        vmaCi.instance = ctx->instance;
        vmaCi.physicalDevice = ctx->physicalDevice;
        vmaCi.vulkanApiVersion = VulkanVersion;
        vmaCi.pVulkanFunctions = &vkFuncs;
        vmaCreateAllocator(&vmaCi, &ctx->allocator);

        auto maxFif = ctx->maxFrameInFlight;
        ctx->descriptorSetMgr = new DescriptorSetManager(ctx,maxFif);
        ctx->cmdBufferMgr = new CommandBufferManager(maxFif);
        ctx->destroyer = new DeferredDestroyer(maxFif);
        ctx->imageDataMgr = new ImageDataManager(maxFif);
        return ctx;
    }

    void deinitVulkanBackEnd(rs_context_vk* ctx)
    {
        if (ctx->computeQueue)
            vkQueueWaitIdle(ctx->computeQueue->queue);
        if(ctx->transferQueue)
            vkQueueWaitIdle(ctx->transferQueue->queue);

        vkQueueWaitIdle(ctx->graphicQueue->queue);

        if(ctx->presentQueue)
            vkQueueWaitIdle(ctx->presentQueue->queue);
        ctx->imageDataMgr->clearAll(ctx);
        ctx->destroyer->clearAll(ctx);
        ctx->cmdBufferMgr->clearAll(ctx);
        delete ctx->cmdBufferMgr;
        delete ctx->destroyer;
        delete ctx->imageDataMgr;
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

    rs_rendertarget_vk* createRsRenderTarget(rs_context_vk* ctx,rs_image_vk** images,int imageNum, rs_image_vk* depthStencil)
    {
        int iw = images[0]->width;
        int ih = images[0]->height;
        int il = images[0]->arrayLayers;

        for (int i = 0; i < imageNum; ++i) {
            const auto img = images[i];
            if (img->width != iw && img->height != ih && img->arrayLayers != il) {
                Log::error("Miss match width/height/array layers in render target.");
                return nullptr;
            }
        }

        std::vector<rs_image*> localimages;

        int depthAttPos = -1;

        for (int i = 0; i < imageNum; ++i) {
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
        rt->rtPassHash = CalcRenderTargetPassHash(ctx, rt);
        return rt;
    }

    void destroyRsRenderTarget(rs_context_vk* ctx, rs_rendertarget_vk*& rt,bool imm)
    {
        if (rt->native != nullptr) {
            if (imm) {
                vkDestroyFramebuffer(ctx->device, (VkFramebuffer)rt->native, 0);
				delete rt;
				rt = 0;
            }
            else {
                ctx->destroyer->destroyRenderTarget(ctx->nextRenderFrame,rt);
            }
        }

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

    rs_buffer_vk* createRsBufferVk(rs_context_vk* context,const BufferDesc& desc)
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
        if (buffer->mappedPtr) {
            vmaUnmapMemory(context->allocator, buffer->allocation);
            buffer->mappedPtr = 0;
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
        buffer->mappedPtr = 0;
    }

    uint64_t getOffsetAllocation(decltype(rs_buffer_vk::allocation) allocation)
    {
        return allocation->GetOffset();
    }

    VkDeviceMemory getDeviceMemory(decltype(rs_buffer_vk::allocation) allocation)
    {
        return allocation->GetMemory();
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

    rs_sampler_vk* createRsSampler(rs_context_vk* ctx,const SamplerDesc& desc)
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
        auto semNative = new VkSemaphore[ctx->maxFrameInFlight];
        sem->native = semNative;
        sem->cnt = ctx->maxFrameInFlight;
        for (int i = 0; i < sem->cnt; ++i) {
            vkCreateSemaphore(ctx->device, &ci, 0, &semNative[i]);
        }
        return sem;
    }

    void destroyRsSemaphore(rs_context_vk* ctx, rs_semaphore_vk*& sem)
    {
        auto semNative = (VkSemaphore*)sem->native;
        for (int i = 0; i < sem->cnt; ++i) {
            vkDestroySemaphore(ctx->device, semNative[i], 0);
        }
        delete[] semNative;
        delete sem;
        sem = 0;
    }

    rs_fence_vk* createRsFence(rs_context_vk* ctx)
    {
        VkFenceCreateInfo ci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        rs_fence_vk* fence = new rs_fence_vk;
        fence->cnt = ctx->maxFrameInFlight;
        VkFence* fenceNative = new VkFence[ctx->maxFrameInFlight];
        fence->native = fenceNative;
        for (int i = 0; i < fence->cnt; ++i) {
            vkCreateFence(ctx->device, &ci, 0, &fenceNative[i]);
        }
        vkResetFences(ctx->device, fence->cnt, fenceNative);
        return fence;
    }

    void destroyRsFence(rs_context_vk* ctx, rs_fence_vk*& fence)
    {
        for (int i = 0; i < fence->cnt; ++i) {
            vkDestroyFence(ctx->device, ((VkFence*)(fence->native))[i], 0);
        }
        delete[] (VkFence*)(fence->native);
        delete fence;
        fence = 0;
    }

    void resetRsFence(rs_context_vk* ctx, rs_fence_vk* fence,int frameInFlight)
    {
        auto fenceNative = (VkFence*)fence->native;
        vkResetFences(ctx->device, 1, &(fenceNative[frameInFlight]));
    }

    void resetRsFence(rs_context_vk* ctx, rs_fence_vk* fence)
    {
        auto fenceNative = (VkFence*)fence->native;
        vkResetFences(ctx->device, fence->cnt, fenceNative);
    }

    void waitForRsFence(rs_context_vk* ctx, rs_fence_vk* fence, uint64_t timeout, int frameInFlight)
    {
        auto fenceNative = ((VkFence*)fence->native)[frameInFlight];
        vkWaitForFences(ctx->device, 1, &fenceNative, VK_TRUE, timeout);
    }

    rs_commandbuffer_vk* createRsCommand(rs_context_vk* ctx, const CommandBufferDesc& desc)
    {
        auto cmdMgr = ctx->cmdBufferMgr;
        return cmdMgr->getCmdBufferLocalThread(ctx, ctx->nextRenderFrame, desc.queueType,desc.transient);
    }

    rs_commandbuffer_vk* createRsCommandTargetFrame(rs_context_vk* ctx, const CommandBufferDesc& desc, uint64_t frame)
    {
        //this function might be called in the render thread, which may have a latency in ctx->maxFrameInFlight frames
        assert( frame >= std::min(0ull, ctx->nextRenderFrame - ctx->maxFrameInFlight));
        auto cmdMgr = ctx->cmdBufferMgr;
        return cmdMgr->getCmdBufferLocalThread(ctx, frame, desc.queueType, desc.transient);
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
        if (oldSwapchain) {
            destroySwapChain(context);
        }
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
        context->swapchain->SwapchainImageFormat = ToImageFormat(choosenFormat.format);
    }

    void destroySwapChain(rs_context_vk* context)
    {
        VkSwapchainKHR swapchain = (VkSwapchainKHR)context->swapchain->native;
        vkDestroySwapchainKHR(context->device,swapchain,0);
        //DestroyViews 
        //Images are created by swapchai  
        auto& views = context->swapchain->swapchainImgs;
        for (auto image : views) {
            vkDestroyImageView(context->device, image->view, 0);
            delete image;
        }
        views.resize(0);
    }

	void cmdsetRenderTarget(rs_context_vk* ctx, rs_commandbuffer_vk* cmd, rs_rendertarget_vk* rt)
	{
        auto curRp = (rs_renderpass_vk*)cmd->currentRenderPass;
        if (!curRp) {
            Log::error("Cannot set rendertarget before begin render pass");
            return;
        }

		if (curRp->passHash != rt->rtPassHash) {
			Log::error("Not compatible render pass for this render target");
			return;
		}

        int width = rt->m_attachments[0]->width;
		int height = rt->m_attachments[0]->height;
        int arrLayer = rt->m_attachments[0]->arrayLayers;


        VkFramebuffer* frameBuffer = (VkFramebuffer*)&rt->native;
        VkRenderPass renderPass = (VkRenderPass)cmd->currentRenderPass->native;
        if (*frameBuffer == nullptr) {
            //Create one 
            std::vector<VkImageView> imageViews;
            imageViews.reserve(rt->m_attachments.size() + rt->m_depthStencilAttachment != nullptr ? 1 : 0);
            for (const auto& i : rt->m_attachments) {
                imageViews.push_back(((rs_image_vk*)i)->view);
            }
            if (rt->m_depthStencilAttachment) {
				imageViews.push_back(((rs_image_vk*)rt->m_depthStencilAttachment)->view);
            }
            VkFramebufferCreateInfo fbCi{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
			fbCi.renderPass = (VkRenderPass)curRp->native;
            fbCi.attachmentCount = imageViews.size();
            fbCi.pAttachments = imageViews.data();
            fbCi.width = width;
			fbCi.height = height;
			fbCi.layers = arrLayer;
            VK_CHECK(vkCreateFramebuffer(ctx->device, &fbCi, nullptr, frameBuffer), { return; });
        }
        if (cmd->currentRenderTarget != NULL) {
            //End this renderpass first 
            cmdEndRenderPass(cmd);
            cmd->currentRenderPass = curRp;
        }
        //Then Begin.
        std::vector<VkClearValue> clearValues;
        clearValues.reserve(rt->m_attachments.size() + rt->m_depthStencilAttachment != nullptr ? 1 : 0);
        for (int i = 0;i < rt->m_attachments.size();++i) {
            const auto& att = curRp->passDesc.attachments[i];
            
            VkClearValue val = {};
            ClearColor curCol = {};
            if (i < cmd->currentClearColor.size())
            {
                if (i < cmd->currentClearColor.size()) {
                    curCol = cmd->currentClearColor[i];
                }
                else {
                    curCol = ClearColor{};
                }

            }
            if (att.isHDR) {
                val.color.float32[0] = curCol.rgba[0];
				val.color.float32[1] = curCol.rgba[1];
				val.color.float32[2] = curCol.rgba[2];
				val.color.float32[3] = curCol.rgba[3];
            }
            else {
				val.color.uint32[0] = uint32_t(curCol.rgba[0] * 255);
				val.color.uint32[1] = uint32_t(curCol.rgba[1] * 255);
				val.color.uint32[2] = uint32_t(curCol.rgba[2] * 255);
				val.color.uint32[3] = uint32_t(curCol.rgba[3] * 255);
            }
            clearValues.push_back(val);
        }

        if (rt->m_depthStencilAttachment) {
			VkClearValue val = {};
            val.depthStencil.depth = cmd->currentClearDepthStencil.depth;
			val.depthStencil.stencil = cmd->currentClearDepthStencil.stencil;
            clearValues.push_back(val);
        }


        VkRenderPassBeginInfo info{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        info.renderPass = renderPass;
        info.framebuffer = *frameBuffer;
        auto& renderArea = info.renderArea;
        renderArea.extent.width = width;
		renderArea.extent.height = height;
        renderArea.offset = {};


		info.             clearValueCount = clearValues.size();
        info.             pClearValues    = clearValues.data();
        cmd->currentRenderTarget = rt;
        vkCmdBeginRenderPass((VkCommandBuffer)(cmd->native), &info, VK_SUBPASS_CONTENTS_INLINE);
	}

	void cmdBeginRenderPass(rs_commandbuffer_vk* cb, rs_renderpass_vk* renderpass, const std::vector<ClearColor>& color, const ClearDepthStencil& clearDs)
	{
        if (cb->currentRenderPass) {
            cmdEndRenderPass(cb);
        }
        cb->currentRenderPass = renderpass;
        cb->currentClearColor = color;
        cb->currentClearDepthStencil = clearDs;
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
        appInfo.apiVersion = VulkanVersion; // 或 VK_API_VERSION_1_0

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
            vkGetPhysicalDeviceProperties(context->physicalDevice, &context->physicalDeviceProperties);

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

    void updateImage(rs_context_vk* ctx, rs_commandbuffer_vk* cmd, rs_image_vk* image, void* data, uint64_t size, int x, int y, int z, int width, int height, int depth, uint32_t mip, uint32_t layeroff, uint32_t layerSize, bool imm)
    {
        auto curFif = ctx->LogicFrameFif;
        if (imm) {
            ctx->imageDataMgr->updateImageData(curFif, ctx, image, data, size, x, y, z, width, height, depth, layeroff, layerSize, mip);
        }
        else {
            ctx->imageDataMgr->cmdUpdateImageData(curFif, ctx, cmd, image, data, size, x, y, z, width, height, depth, layeroff, layerSize, mip);
        }
    }

    void updateBuffer(rs_context_vk* ctx, rs_commandbuffer_vk* cmd, rs_buffer_vk* buffer, void* data, uint64_t size, uint32_t offsetDst, bool imm)
    {
        auto curFif = ctx->LogicFrameFif;
        if (imm) {
            ctx->imageDataMgr->updateBufferData(curFif, ctx, buffer, data, size, offsetDst);
        }
        else {
            ctx->imageDataMgr->cmdUpdateBufferData(curFif, ctx, cmd, buffer, data, size, offsetDst);
        }
    }

    rs_drawdata_vk* createDrawData(rs_context_vk* context)
    {
        auto drawData = new rs_drawdata_vk;
        drawData->DescriptorSets.resize(context->maxFrameInFlight);
        return drawData;
    }

    void clearDrawData(rs_context_vk* ctx, rs_drawdata_vk* drawdata)
    {
        std::vector <			//For each Frame
            std::vector<		//Each Descriptorset
            std::pair<uint16_t, rs_descriptorSet_vk*>
            >
        > DescriptorSets;
        for (auto& frame : drawdata->DescriptorSets) {
            for (auto& [setIdx, descriptorset] : frame) {
                ctx->descriptorSetMgr->ReturnDescriptorSet(ctx,descriptorset);
            }
        }
    }

    void destroyDrawData(rs_context_vk* context, rs_drawdata_vk* drawdata)
    {
        for (auto&& frame : drawdata->DescriptorSets) {
            for (auto& [idx, descriptor] : frame) {
                context->descriptorSetMgr->ReturnDescriptorSet(context, descriptor);
            }
        }
        delete drawdata;
    }
    inline rs_descriptorSet_vk* _findOrCreateDescripotrSet(rs_context_vk* context,uint64_t frame, uint32_t fif,rs_pipeline_vk* pipeline, rs_drawdata_vk* drawdata, uint32_t vkSet) {
        auto& frameSets = drawdata->DescriptorSets[fif];
        rs_descriptorSet_vk* targetSet = 0;
        for (auto& [idx, descriptor] : frameSets) {
            if (idx == vkSet) {
                targetSet = descriptor;
                break;
            }
        }

        if (!targetSet) {
            targetSet = context->descriptorSetMgr->AllocateDescriptorSet(frame, context, pipeline, vkSet);
            if (!targetSet) {
                assert(0);
            }
            frameSets.push_back({ vkSet,targetSet });
        }

        return targetSet;

    }

    void updateDrawData(rs_context_vk* context, uint64_t frame, rs_pipeline_vk* pipeline, rs_drawdata_vk* drawdata, rs_binding_pos pos, void* data, size_t size)
    {
        auto vkPos = toVkBindingPos(pos);
        auto fif = context->LogicFrameFif;

        auto descriptorSet = _findOrCreateDescripotrSet(context, frame, fif, pipeline, drawdata, vkPos.setIdx);
        if (descriptorSet) {
            context->descriptorSetMgr->updateBufferData(frame, context, descriptorSet, vkPos.bindingIdx, data, size, QueueType_Graphics);
        }
        else {
            Log::error("Update descripotSet error");
        }

    }

    void updateDrawData(rs_context_vk* context, uint64_t frame, rs_pipeline_vk* pipeline, rs_drawdata_vk* drawdata, rs_binding_pos pos, rs_image_vk* image)
    {
        auto vkPos = toVkBindingPos(pos);
        auto fif = context->LogicFrameFif;
        auto descriptorSet = _findOrCreateDescripotrSet(context, frame, fif, pipeline, drawdata, vkPos.setIdx);
        if (descriptorSet) {
            context->descriptorSetMgr->updateImage(frame, context, descriptorSet, vkPos.bindingIdx, image, QueueType_Graphics);
        }else {
            Log::error("Update descripotSet error");
        }
    }

    void updateDrawData(rs_context_vk* context, uint64_t frame, rs_pipeline_vk* pipeline, rs_drawdata_vk* drawdata, rs_binding_pos pos, rs_sampler_vk* vk)
    {
        auto vkPos = toVkBindingPos(pos);
        auto fif = context->LogicFrameFif;
        auto descriptorSet = _findOrCreateDescripotrSet(context, frame, fif, pipeline, drawdata, vkPos.setIdx);
        if (descriptorSet) {
            context->descriptorSetMgr->updateSampler(frame, context, descriptorSet, vkPos.bindingIdx, vk, QueueType_Graphics);
        }
        else {
            Log::error("Update descripotSet error");
        }
    }

    void updateDrawData(rs_context_vk* context, uint64_t frame, rs_pipeline_vk* pipeline, rs_drawdata_vk* drawdata, rs_binding_pos pos, rs_buffer_vk* vk)
    {
        auto vkPos = toVkBindingPos(pos);
        auto fif = context->LogicFrameFif;
        auto descriptorSet = _findOrCreateDescripotrSet(context, frame, fif, pipeline, drawdata, vkPos.setIdx);
        if (descriptorSet) {
            context->descriptorSetMgr->updateBuffer(frame, context, descriptorSet, vkPos.bindingIdx, vk, QueueType_Graphics);
        }
        else {
            Log::error("Update descripotSet error");
        }
    }

    rs_buffer_vk* createStageBufferTemp(rs_context_vk* context, uint64_t size)
    {
        BufferDesc stageBufferDesc{};
        stageBufferDesc.bufUsage = BufferType_TransferSrc;
        stageBufferDesc.mappable = true;
        stageBufferDesc.queueType = QueueType_Graphics;
        stageBufferDesc.byteSize = size;
        return Vulkan::createRsBufferVk(context, stageBufferDesc);
    }


    void cmdEndRenderPass(rs_commandbuffer_vk* cb)
    {
        vkCmdEndRenderPass((VkCommandBuffer)cb->native);

        auto currentRenderPass = cb->currentRenderPass;
        auto rt = cb->currentRenderTarget;
        auto& images = rt->m_attachments;
        int idx = 0;
        for (auto image : images) {
            //Get Current Real Image layout
            auto imgVk = (rs_image_vk*)image;
            auto& rpDesc = currentRenderPass->passDesc;

            imgVk->currentLayout = pickLayout(imgVk->usage, rpDesc.attachments[idx].storeOp);
            //patch for present src, it will always be transfer into present src layout by renderpass.
            if ((image->usage & ImageUsage_PresentSrc)) {
                imgVk->currentLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            }

            //patch for present src, it will always be transfer into present src layout by renderpass.
            if (image->usage & ImageUsage_Sampled && ((image->usage & ImageUsage_PresentSrc) == 0)) {
                cmdImageLayoutTo(cb, 
                    (rs_image_vk*)image, 
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    0, image->mipLevels,
                    0, image->arrayLayers,
                    (uint32_t)(VK_IMAGE_ASPECT_COLOR_BIT)
                );
            }
            idx++;
        }

        if (rt->m_depthStencilAttachment) {
            auto& image = rt->m_depthStencilAttachment;
            if (image->usage & ImageUsage_Sampled) {
                cmdImageLayoutTo(cb,
                    (rs_image_vk*)image,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    0, image->mipLevels,
                    0, image->arrayLayers,
                    (uint32_t)(VK_IMAGE_ASPECT_DEPTH_BIT)
                );
            }
        }
        cb->currentRenderTarget = 0;
        cb->currentRenderPass = 0;
    }

    void cmdSetViewport(rs_commandbuffer_vk* cb,const Rect2D& rect, float minDepth, float maxDepth, uint32_t idx)
    {
        assert(cb->currentRenderPass != nullptr);
        VkViewport viewport{};
        auto& rt = cb->currentRenderTarget;
        int width = rt->m_attachments[0]->width;
        int height = rt->m_attachments[0]->height;
        int arrLayer = rt->m_attachments[0]->arrayLayers;
        viewport.x = rect.l * width;
        viewport.y = rect.t * height;
        viewport.width = (rect.r - rect.l) * width;
        viewport.height = (rect.b - rect.t) * height;
        viewport.minDepth = minDepth;
        viewport.maxDepth = maxDepth;
        vkCmdSetViewport((VkCommandBuffer)cb->native, idx, 1, &viewport);
    }

    void cmdSetScissor(rs_commandbuffer_vk* cb,const Rect2D& rect, uint32_t idx)
    {
        assert(cb->currentRenderPass != nullptr);
        auto& rt = cb->currentRenderTarget;
        int width = rt->m_attachments[0]->width;
        int height = rt->m_attachments[0]->height;
        int arrLayer = rt->m_attachments[0]->arrayLayers;
        
        VkRect2D scissor{};
        scissor.extent.width = (rect.r - rect.l) * width;
        scissor.extent.height = (rect.b - rect.t) * height;
        scissor.offset.x = rect.l * width;
        scissor.offset.y = rect.t * height;

        vkCmdSetScissor((VkCommandBuffer)cb->native, idx, 1, &scissor);
    }

    void cmdDrawIndexed(rs_commandbuffer_vk* cb, rs_pipeline_vk* pipeline, const RenderInfo& info, std::array<rs_drawdata_vk*, 3> drawDatas, uint32_t curFif, bool isInstanced)
    {
        if (pipeline == 0) {
            assert(0);
            return;
        }
        
        auto cmd = (VkCommandBuffer)cb->native;

        auto& bufferBinding = pipeline->vtxInput.bindings;
        if (bufferBinding.size() != info.bindingBuffers.size()) {
            assert(0);
            return;
        }
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipeline)pipeline->native);
        //It's ok to draw with out index buffer
        bool donotuseidxdraw = false;
        if (info.indexBuffer) {
            vkCmdBindIndexBuffer(cmd, (VkBuffer)info.indexBuffer->native, info.idxOffset, info.indexType == IndexType::Uint16 ? VkIndexType::VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
        }
        else {
            donotuseidxdraw = true;
        }
        std::vector<VkBuffer> bindingBuffers;
        std::vector<VkDeviceSize> bufferoffsets;
        for (int i = 0; i < info.bindingBuffers.size(); ++i) {
            bindingBuffers.push_back((VkBuffer)(info.bindingBuffers[i].buffer->native));
            bufferoffsets.push_back(info.bindingBuffers[i].offset);
        }
        if (bufferBinding.size() > 0) {
            vkCmdBindVertexBuffers(cmd, 0, bufferBinding.size(), bindingBuffers.data(), bufferoffsets.data());
        }
        VkPipelineLayout pLayout = (VkPipelineLayout)(((rs_pipeline_vk*)pipeline)->layout->native);

        for (auto&& drawdata : drawDatas) {
            if (drawdata) {
                cmdBindDrawData(cb, pLayout, drawdata, curFif);
            }
        }

        uint32_t instanceCnt = isInstanced ? info.instanceCount : 1;
        if (donotuseidxdraw) {
            vkCmdDraw(cmd, info.idxCount, instanceCnt, info.vtxoffset, 0);
        }
        else {
            vkCmdDrawIndexed(cmd, info.idxCount, instanceCnt, 0, info.vtxoffset, 0);
        }
    }

    void cmdDrawIndexed(rs_commandbuffer_vk* cb, rs_pipeline_vk* pipeline, const RenderInfo& info, rs_drawdata_vk* drawData, uint32_t curFif, bool isInstanced)
    {
        if (pipeline == 0) {
            assert(0);
            return;
        }
        auto& bufferBinding = pipeline->vtxInput.bindings;
        if (bufferBinding.size() != info.bindingBuffers.size()) {
            assert(0);
            return;
        }
        auto cmd = (VkCommandBuffer)cb->native;
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipeline)pipeline->native);


        bool donotuseidxdraw = false;
        if (info.indexBuffer) {
            vkCmdBindIndexBuffer(cmd, (VkBuffer)info.indexBuffer->native, info.idxOffset, info.indexType == IndexType::Uint16 ? VkIndexType::VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
        }
        else {
            donotuseidxdraw = true;
        }

        std::vector<VkBuffer> bindingBuffers;
        std::vector<VkDeviceSize> bufferoffsets;
        for (int i = 0; i < info.bindingBuffers.size(); ++i) {
            bindingBuffers.push_back((VkBuffer)(info.bindingBuffers[i].buffer->native));
            bufferoffsets.push_back(info.bindingBuffers[i].offset);
        }
        if (bufferBinding.size() > 0) {
            vkCmdBindVertexBuffers(cmd, 0, bufferBinding.size(), bindingBuffers.data(), bufferoffsets.data());
        }
        VkPipelineLayout pLayout = (VkPipelineLayout)(((rs_pipeline_vk*)pipeline)->layout->native);

        cmdBindDrawData(cb, pLayout, drawData, curFif);

        uint32_t instanceCnt = isInstanced ? info.instanceCount : 1;
        if (donotuseidxdraw) {
            vkCmdDraw(cmd, info.idxCount, instanceCnt, info.vtxoffset,0);
        }
        else {
            vkCmdDrawIndexed(cmd, info.idxCount, instanceCnt, 0, info.vtxoffset, 0);
        }
    }

    void cmdBindDrawData(rs_commandbuffer_vk* cb, VkPipelineLayout pipelineLayout, rs_drawdata_vk* drawData, uint32_t curFif)
    {
        static const int maxDynamicSize = 32;
        std::array<uint32_t, maxDynamicSize> dynamics;
        int dyNum = 0;
        auto& curFrameDescriptors = ((rs_drawdata_vk*)drawData)->DescriptorSets[curFif];
        for (const auto& [setIdx, descriptorSet] : curFrameDescriptors) {
            auto descriptorSetVk = (VkDescriptorSet)descriptorSet->native;
            for (auto&& binding : descriptorSet->mBindingData) {
                if (binding.type == ResourceType::UniformBuffer) {
                    dynamics[dyNum] = (binding.uboDyOffset);
                    dyNum += 1;
                }
            }
            vkCmdBindDescriptorSets((VkCommandBuffer)cb->native, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, setIdx, 1, &descriptorSetVk,dyNum, dynamics.data());
        }
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
        cb->hasCommands = true;
        uint64_t cpSize = std::min((uint64_t)buffer->byteSize, size);
        if (buffer->mappedPtr) {
            memcpy((uint8_t*)buffer->mappedPtr + dstOffset, data, size);
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
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 1, &barrierBefore, 0, 0);
        vkCmdCopyBuffer2(cmd,&cpInfo);
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, 0, 1, &barrierAfter, 0, 0);
        destroyRsBuffer(ctx,tempBuffer);

    }

    void cmdCopyBufferToBuffer(rs_commandbuffer_vk* cb, rs_context_vk* context, rs_buffer_vk* bufferSrc, rs_buffer_vk* bufferDst, uint64_t size, uint64_t srcOffset, uint64_t dstOffset)
    {
        cb->hasCommands = true;
        VkCopyBufferInfo2 cpInfo{
    VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2
        };

        cpInfo.srcBuffer = (VkBuffer)bufferSrc->native;
        cpInfo.dstBuffer = (VkBuffer)bufferDst->native;
        cpInfo.regionCount = 1;
        VkBufferCopy2 bufferCopy{
            VK_STRUCTURE_TYPE_BUFFER_COPY_2
        };
        cpInfo.pRegions = &bufferCopy;

        bufferCopy.srcOffset = srcOffset;
        bufferCopy.dstOffset = dstOffset;
        bufferCopy.size = size;

        VkBufferMemoryBarrier barrierBefore = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT & VK_ACCESS_TRANSFER_READ_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = (VkBuffer)bufferSrc->native,
        .offset = 0,
        .size = VK_WHOLE_SIZE,
        };

        VkBufferMemoryBarrier barrierAfter = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = bufferBarrierTransferDstCalc(bufferDst),
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = (VkBuffer)bufferDst->native,
        .offset = dstOffset,
        .size = VK_WHOLE_SIZE,
        };

        auto cmd = (VkCommandBuffer)cb->native;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 1, &barrierBefore, 0, 0);
        vkCmdCopyBuffer2(cmd, &cpInfo);
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, 0, 1, &barrierAfter, 0, 0);
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
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
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

    void cmdImageLayoutTo(rs_commandbuffer_vk* cb, rs_image_vk* image, VkImageLayout newlayout, uint32_t mip, uint32_t mipSize, uint32_t layeroff, uint32_t layersize, uint32_t aspect)
    {
        if (image->currentLayout == newlayout)return;
        cmdImageLayoutTo(cb, image, image->currentLayout, newlayout, mip, mipSize, layeroff, layersize, aspect);
        image->currentLayout = newlayout;
    }

    void cmdImageLayoutTo(rs_commandbuffer_vk* cb, rs_image_vk* image, VkImageLayout fromlayout, VkImageLayout newlayout, uint32_t mip, uint32_t mipSize, uint32_t layeroff, uint32_t layersize, uint32_t aspect)
    {
        VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = calcSrcImageLayoutAccessStage(fromlayout),  // 前一个布局带来的访问屏障
        .dstAccessMask = calcDstImageLayoutAccessStage(newlayout),
        .oldLayout = fromlayout,                             // 比如：VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        .newLayout = newlayout,  // 要切到 transfer dst
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = (VkImage)image->native,
        .subresourceRange = {
            .aspectMask = aspect,
            .baseMipLevel = mip,   // 只针对你要写入的 mip 级
            .levelCount = mipSize,
            .baseArrayLayer = layeroff,   // 只针对你要写入的那一层
            .layerCount = layersize,
        },

        };
        vkCmdPipelineBarrier((VkCommandBuffer)cb->native, getSrcStageForLayout(image->currentLayout), getDstStageForLayout(newlayout), 0, 0, 0, 0, 0, 1, &barrier);

    }

    void cmdSubmitCmdBuffer(rs_context_vk* ctx, rs_commandbuffer_vk* cb, QueueType queue, std::vector<rs_semaphore*> imageAvailableWaitSemaphores, std::vector<rs_semaphore*> renderFinishSignalSemphores, rs_fence_vk* fence)
    {

        auto curFif = ctx->RenderFrameFif;
        std::vector<VkSemaphore> ntvwaitSemaphores,ntvsignalSemaphores;
        for (auto&& sem : imageAvailableWaitSemaphores) {
            ntvwaitSemaphores.push_back( ((VkSemaphore*)sem->native)[curFif]);
        }
        for (auto&& sem : renderFinishSignalSemphores) {
            ntvsignalSemaphores.push_back(((VkSemaphore*)sem->native)[curFif]);
        }

        VkPipelineStageFlags toWaitFlag;

        switch (queue)
        {
        case Render::QueueType_Graphics:
            toWaitFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        case Render::QueueType_Compute:
            toWaitFlag = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            break;
        case Render::QueueType_Transfer:
            toWaitFlag = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case Render::QueueType_Present:
        default:
            toWaitFlag = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
            break;
        }

        std::vector< VkPipelineStageFlags> waitStageFlags(imageAvailableWaitSemaphores.size(),toWaitFlag);

        VkSubmitInfo info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };

        info.                       waitSemaphoreCount = ntvwaitSemaphores.size();
        info.                       pWaitSemaphores = ntvwaitSemaphores.data();
        info.commandBufferCount = 1;
        info.pCommandBuffers = (VkCommandBuffer*)&cb->native;
        info.signalSemaphoreCount = ntvsignalSemaphores.size();
        info.pSignalSemaphores = ntvsignalSemaphores.data();
        info.pWaitDstStageMask = waitStageFlags.data();
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
        VkFence fencevk = VK_NULL_HANDLE;
        if (fence) {
            fencevk = ((VkFence*)fence->native)[curFif];
        }
        vkQueueSubmit(rsQueue->queue, 1, &info, fencevk);
    }

    void cmdBeginMark(rs_commandbuffer_vk* cb, const char* mark, float r, float g, float b, float a)
    {
        VkDebugUtilsLabelEXT debugInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
        debugInfo.color[0] = r;
        debugInfo.color[1] = g;
        debugInfo.color[2] = b;
        debugInfo.color[3] = a;
        debugInfo.pLabelName = mark;
        vkCmdBeginDebugUtilsLabelEXT((VkCommandBuffer)cb->native, &debugInfo);
    }

    void cmdEndMark(rs_commandbuffer_vk* cb)
    {
        vkCmdEndDebugUtilsLabelEXT((VkCommandBuffer)cb->native);
    }

    void cmdInsertMark(rs_commandbuffer_vk* cb, const char* mark, float r, float g, float b, float a)
    {
        VkDebugMarkerMarkerInfoEXT debugInfo{VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT};
        debugInfo.color[0] = r;
        debugInfo.color[1] = g;
        debugInfo.color[2] = b;
        debugInfo.color[3] = a;
        debugInfo.pMarkerName = mark;
        vkCmdDebugMarkerInsertEXT((VkCommandBuffer)cb->native, &debugInfo);
    }

    void cmdBeginRecord(rs_commandbuffer_vk* cb)
    {
        VkCommandBufferBeginInfo beginCi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginCi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer((VkCommandBuffer)cb->native, &beginCi);
    }

    void cmdEndRecord(rs_commandbuffer_vk* cb)
    {
        vkEndCommandBuffer((VkCommandBuffer)cb->native);
    }

    uint64_t beginRsFrameVk(rs_context_vk* ctx)
    {
        ctx->nextRenderFrame++;
        uint64_t maxFif = ctx->maxFrameInFlight;
        auto cmdbufMgr = ctx->cmdBufferMgr;
        auto descriptorSetMgr = ctx->descriptorSetMgr;
        //Wait newRenderFrame % maxFif finish
        uint64_t toWaitFrame = ctx->nextRenderFrame % maxFif;
        ctx->LogicFrameFif = toWaitFrame;
        ctx->canRenderNextFrame = true;
        ctx->destroyer->endFrameDestroy(ctx, ctx->curRenderFrame);
        cmdbufMgr->beginFrame(ctx, ctx->nextRenderFrame);
        descriptorSetMgr->beginFrame(ctx, ctx->nextRenderFrame);
        return 0;
    }

    uint64_t beginRsRenderFrameVk(rs_context_vk* ctx)
    {
        ctx->curRenderFrame = ctx->nextRenderFrame;
        uint64_t maxFif = ctx->maxFrameInFlight;
        ctx->RenderFrameFif = ctx->curRenderFrame % maxFif;
        auto imageDataMgr = ctx->imageDataMgr;
        imageDataMgr->beginRenderFrame(ctx->curRenderFrame, ctx);
        return 0;
    }

    uint64_t endRsFrameVk(rs_context_vk* ctx)
    {
        return ctx->nextRenderFrame;
    }

    uint64_t waitForNextPresentImage(rs_context_vk* ctx, rs_semaphore_vk* imageAvailableSignalSemaphore, rs_fence_vk* fenceToSignal)
    {
        uint32_t swapImageIdx;
        VkSemaphore sem = imageAvailableSignalSemaphore == 0 ? VK_NULL_HANDLE : ((VkSemaphore*)imageAvailableSignalSemaphore->native)[ctx->LogicFrameFif];
        VkFence fence = fenceToSignal == 0 ? VK_NULL_HANDLE : ((VkFence*)fenceToSignal->native)[ctx->LogicFrameFif];
        vkAcquireNextImageKHR(ctx->device, (VkSwapchainKHR)ctx->swapchain->native, 100000000,sem , fence, &swapImageIdx);
        return swapImageIdx;
    }

    void submitToPresentImage(rs_context_vk* ctx, uint32_t presentImgIdx, std::vector<rs_semaphore_vk*> renderFinishWaitSemaphore)
    {
        std::vector<VkSemaphore> semphoresToWait;
        for (auto&& semRs : renderFinishWaitSemaphore) {
            semphoresToWait.push_back( ((VkSemaphore*)semRs->native)[ctx->LogicFrameFif]);
        }
        VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount;
        presentInfo.pWaitSemaphores = semphoresToWait.data();
        presentInfo.waitSemaphoreCount = semphoresToWait.size();
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = (VkSwapchainKHR*)&ctx->swapchain->native;
        presentInfo.pImageIndices = &presentImgIdx;
        vkQueuePresentKHR(ctx->presentQueue->queue, &presentInfo);
    }

    void WaitForDeviceIdel(rs_context_vk* ctx)
    {
        vkDeviceWaitIdle(ctx->device);
    }

    void cmdUpdateImage(rs_commandbuffer_vk* cb, rs_context_vk* context, rs_image_vk* image, void* data, uint64_t size, int x, int y, int z, int width, int height, int depth, uint32_t dstMip, int layeroff, int layerSize)
    {
        assert(image && data);
        auto cmd = (VkCommandBuffer)cb->native;
        auto tempBuffer = createStageBufferTemp(context, size);
        auto dstPtr = mapRsBuffer(context, tempBuffer);
        memcpy(dstPtr, data, size);
        cmdUpdateImage(cb, context, image, tempBuffer, x, y, z, width, height, depth, dstMip, layeroff,layerSize);
        destroyRsBuffer(context, tempBuffer);
    }

    void cmdUpdateImage(rs_commandbuffer_vk* cb, rs_context_vk* context, rs_image_vk* image, rs_buffer_vk* pendingBuffer , int x, int y, int z, int width, int height, int depth, uint32_t dstMip, int layeroff, int layerSize)
    {
        auto cmd = (VkCommandBuffer)cb->native;
        VkImageSubresourceLayers    imageSubresource{};
        imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageSubresource.mipLevel = dstMip;
        imageSubresource.baseArrayLayer = layeroff;
        imageSubresource.layerCount = layerSize;
        VkOffset3D                  imageOffset{ x,y,z };
        VkExtent3D                  imageExtent{ width,height,depth };
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
        .buffer = (VkBuffer)pendingBuffer->native,
        .offset = 0,
        .size = VK_WHOLE_SIZE,
        };

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 1, &barrierBefore, 0, 0);
        auto layoutSave = image->currentLayout;
        cmdImageLayoutTo(cb, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstMip,1, layeroff,layerSize, VK_IMAGE_ASPECT_COLOR_BIT);
        VkCopyBufferToImageInfo2 cpInfo{ VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2 };
        cpInfo.srcBuffer = (VkBuffer)pendingBuffer->native;
        cpInfo.dstImage = (VkImage)image->native;
        cpInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        cpInfo.regionCount = 1;
        cpInfo.pRegions = &imageCpy;
        vkCmdCopyBufferToImage2(cmd, &cpInfo);
        cmdImageLayoutTo(cb, image, layoutSave, dstMip,1, layeroff, layerSize, VK_IMAGE_ASPECT_COLOR_BIT);

    }

}


