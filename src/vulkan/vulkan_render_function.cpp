#define VK_NO_PROTOTYPES
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION
#include "volk.h"
#include <cmath>
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
#include "vulkan/vulkan_resource_state.h"
#include "vulkan/vulkan_global_def.h"
#include "vulkan/vulkan_resource_state.h"
#include "render_function.h"
#include "render_log.h"
#include <set>
#include <iostream>
#include "Renderer/GPUShared/BindlessGlobalDefShared.h"
#include <common/BindlessIndexingTable.h>
#include "render_resource_global.h"



namespace {
    //validation callback
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT              messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {
        switch (messageTypes)
        {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT :
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
        uint32_t desired = caps.minImageCount + 1;
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
    std::pair<rs_descriptorSet_vk*,descriptor_set_pack*> _findOrCreateDescripotrSet(rs_context_vk* context, uint64_t frame, uint32_t fif, rs_graphic_pipeline_vk* pipeline, rs_drawdata_vk* drawdata, uint32_t vkSet);

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
        case VK_FORMAT_R16G16B16_SFLOAT:        return ImageFormat::RGB16_SFLOAT;
		case VK_FORMAT_B10G11R11_UFLOAT_PACK32: return ImageFormat::R11G11B10_UFLOAT;
		case VK_FORMAT_R16G16B16A16_SFLOAT:     return ImageFormat::RGBA16_SFLOAT;

        case VK_FORMAT_R32_SFLOAT:              return ImageFormat::R32_SFLOAT;
        case VK_FORMAT_R32G32_SFLOAT:           return ImageFormat::RG32_SFLOAT;
        case VK_FORMAT_R32G32B32_SFLOAT:    return ImageFormat::RGB32_SFLOAT;
        case VK_FORMAT_R32G32B32A32_SFLOAT:     return ImageFormat::RGBA32_SFLOAT;
        case VK_FORMAT_R32G32B32A32_UINT:      return ImageFormat::RGBA32_UINT;

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
        case ImageFormat::RGB32_SFLOAT:       return VK_FORMAT_R32G32B32_SFLOAT;
		case ImageFormat::R11G11B10_UFLOAT:   return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
		case ImageFormat::RGBA32_SFLOAT:      return VK_FORMAT_R32G32B32A32_SFLOAT;

        case ImageFormat::RGBA32_UINT:        return VK_FORMAT_R32G32B32A32_UINT;

        case ImageFormat::D16_UNORM:          return VK_FORMAT_D16_UNORM;
        case ImageFormat::D24_UNORM_S8_UINT:  return VK_FORMAT_D24_UNORM_S8_UINT;
        case ImageFormat::D32_SFLOAT:         return VK_FORMAT_D32_SFLOAT;
        case ImageFormat::D32_SFLOAT_S8_UINT: return VK_FORMAT_D32_SFLOAT_S8_UINT;

        default:                              return VK_FORMAT_UNDEFINED;
        }
    }

	VkImageSubresourceRange toVkImageSubresourceRange(const ImageViewKey& viewKey) {
		VkImageSubresourceRange subresourceRange{};

		subresourceRange.aspectMask = fromEngineAspecttoVkAspect(viewKey.getAspect());

		subresourceRange.baseMipLevel = viewKey.getBaseMip();
		subresourceRange.levelCount = viewKey.getMipCount();

		subresourceRange.baseArrayLayer = viewKey.getBaseLayer();
		subresourceRange.layerCount = viewKey.getLayerCount();

		return subresourceRange;
	}

    void queryAllImageFormatCaps(rs_context_vk* ctx)
    {
        auto physDev = ctx->physicalDevice;

        for (int i = 0; i < int(ImageFormat::Invalid); ++i)
        {
            ImageFormat imgFmt = static_cast<ImageFormat>(i);
            VkFormat vkFmt = toVkFormat(imgFmt);

            FormatCapFlag caps = 0;

            if (vkFmt == VK_FORMAT_UNDEFINED)
            {
                ctx->ImageFormatCaps[i] = caps;
                continue;
            }

            VkFormatProperties props{};
            vkGetPhysicalDeviceFormatProperties(physDev, vkFmt, &props);

            VkFormatFeatureFlags features = props.optimalTilingFeatures;

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

		auto isFormatSupported = [ctx](ImageFormat fmt, bool depthStencil) -> bool {
            uint32_t queryFlags = depthStencil ? ImgFormatCaps::Supported | ImgFormatCaps::DepthStencil : ImgFormatCaps::Supported | ImgFormatCaps::ColorAtt;
            return Render::queryImgFormatCaps(ctx, fmt, queryFlags);
			};

        auto SetImageFormat = [&](RenderTextureFormat RTFormat, ImageFormat TargetFormat,bool depthStencil,const std::vector<ImageFormat>& fallbacks) {
            if (isFormatSupported(TargetFormat, depthStencil)) {
                map[(int)RTFormat] = TargetFormat;
                return;
            }
            else {
                Log::warn("Render texture format " + std::to_string((int)RTFormat) + " is not supported, fallback happened...");
            }

            for (auto& color : fallbacks) {
                if (isFormatSupported(color, depthStencil)) {
					map[(int)RTFormat] = color;
                    return;
                }
            }
        };


        // ------------------------
        // LDR Color
        // ------------------------
        SetImageFormat(RenderTextureFormat::RGBA8, ImageFormat::RGBA8_UNORM, false, { ImageFormat::BGRA8_UNORM });

        // ------------------------
        // HDR Color
        // ------------------------
		SetImageFormat(RenderTextureFormat::R16F, ImageFormat::R16_SFLOAT, false, {  });

		SetImageFormat(RenderTextureFormat::RG16F, ImageFormat::RG16_SFLOAT, false, {  });

		SetImageFormat(RenderTextureFormat::R11G11B10F, ImageFormat::R11G11B10_UFLOAT,false , { ImageFormat::RGBA32_SFLOAT });
		
        SetImageFormat(RenderTextureFormat::RGBA16F, ImageFormat::RGBA16_SFLOAT, false, { ImageFormat::RGBA32_SFLOAT });

		SetImageFormat(RenderTextureFormat::RGBA32F, ImageFormat::RGBA32_SFLOAT,false, { ImageFormat::RGBA16_SFLOAT });

        // ------------------------
        // Depth / Stencil
        // ------------------------
		SetImageFormat(RenderTextureFormat::D24S8, ImageFormat::D24_UNORM_S8_UINT,true, { ImageFormat::D32_SFLOAT_S8_UINT, ImageFormat::D24_UNORM_S8_UINT, ImageFormat::D32_SFLOAT });
		SetImageFormat(RenderTextureFormat::D32S8, ImageFormat::D32_SFLOAT_S8_UINT,true, { ImageFormat::D32_SFLOAT_S8_UINT, ImageFormat::D24_UNORM_S8_UINT, ImageFormat::D32_SFLOAT });

		SetImageFormat(RenderTextureFormat::D32, ImageFormat::D32_SFLOAT,true, { ImageFormat::D32_SFLOAT_S8_UINT, ImageFormat::D24_UNORM_S8_UINT, ImageFormat::D32_SFLOAT });

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
            case Filter ::Cubic:
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

		//For first frame, set currentSwapchainImage to the last image, 
        // so that acquire next image will get the first one (index 0) 
        ctx->currentSwapchainImage = ctx->maxSwapChainImages - 1;


        VmaVulkanFunctions vkFuncs{};
        vkFuncs.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vkFuncs.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
        VmaAllocatorCreateInfo vmaCi{};
        if (BufferDeviceAddressEnable) {
            vmaCi.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        }
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

        ctx->semphoreToSignalPresent = createRsSemaphore(ctx);

        return ctx;
    }

    void createDefaultResources(rs_context_vk* ctx)
    {
		defalut_no_sampler = createRsSampler(ctx, SamplerDesc{});
        BufferDesc descBuf{};
        descBuf.byteSize = 8;
        descBuf.bufUsage = BufferType::BufferType_Uniform | BufferType::BufferType_TransferSrc;
        descBuf.mappable = true;
		defalut_no_buffer = createRsBufferVk(ctx, descBuf);
        uint64_t data = 0xFFFFFFFFFFFFFFFF;
        mapRsBuffer(ctx, (rs_buffer_vk*)defalut_no_buffer);
		memcpy(defalut_no_buffer->mappedPtr, &data, 8);
        
        descBuf.bufUsage = BufferType::BufferType_Storage;
        descBuf.mappable = false;
        defalut_no_buffer_UAV = (rs_buffer_vk*)createRsBufferVk(ctx, descBuf);
		auto cmdBuffer = ctx->cmdBufferMgr->getCmdBufferLocalThread(ctx, ctx->maxFrameInFlight, QueueType_Graphics, true);
        cmdBeginRecord(cmdBuffer);
        transitionBufferState(cmdBuffer, (rs_buffer_vk*)defalut_no_buffer, ResourceState::TransferSrc);
        transitionBufferState(cmdBuffer, (rs_buffer_vk*)defalut_no_buffer_UAV, ResourceState::TransferDst);
		cmdCopyBufferToBuffer(cmdBuffer,ctx, (rs_buffer_vk*)defalut_no_buffer, (rs_buffer_vk*)defalut_no_buffer_UAV, 8, 0, 0);
        ImageDesc descImg{};
        descImg.arrayLayers = 1;
        descImg.mipLevels = 1;
		descImg.usage = ImageUsage_Sampled | ImageUsage_TransferDst;
        descImg.width = 1;
        descImg.height = 1;
        descImg.depth = 1;
        descImg.format = ImageFormat::BGRA8_UNORM;

		defalut_no_texture = (rs_image_vk*)createRsImage(ctx, descImg);
		descImg.usage = ImageUsage_Storage | ImageUsage_TransferDst;
        defalut_no_texture_UAV = (rs_image_vk*)createRsImage(ctx, descImg);
        auto tempBuffer = createStageBufferTemp(ctx, 4);
		uint32_t imageRGBA = 0xFF0000FF; // Opaque red
        mapRsBuffer(ctx, tempBuffer);
        memcpy(tempBuffer->mappedPtr, &imageRGBA, 4);
		transitionBufferState(cmdBuffer, tempBuffer, ResourceState::TransferSrc);
		transitionImageState(cmdBuffer, (rs_image_vk*)defalut_no_texture, ResourceState::TransferDst);
		transitionImageState(cmdBuffer,  (rs_image_vk*)defalut_no_texture_UAV, ResourceState::TransferDst);
        cmdCopyBufferToImage(cmdBuffer,ctx,  (rs_image_vk*)defalut_no_texture, tempBuffer, 0, 0, 0, 0, 1, 1,1,0,0,1);
        cmdCopyBufferToImage(cmdBuffer, ctx,  (rs_image_vk*)defalut_no_texture_UAV, tempBuffer, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1);
        transitionImageState(cmdBuffer, (rs_image_vk*)defalut_no_texture, ResourceState::ShaderResource);
        transitionImageState(cmdBuffer, (rs_image_vk*)defalut_no_texture_UAV, ResourceState::ComputeUnorderedAccess);
        destroyRsBuffer(ctx, tempBuffer);
        cmdEndRecord(cmdBuffer);
        cmdSubmitOneShotAndWait(ctx, cmdBuffer);
    }

    void destroyDefaultResources(rs_context_vk* ctx)
    {
        auto sampler = (rs_sampler_vk*)defalut_no_sampler;
		auto buffer = (rs_buffer_vk*)defalut_no_buffer;
		auto bufferUAV = (rs_buffer_vk*)defalut_no_buffer_UAV;
		auto texture = (rs_image_vk*)defalut_no_texture;
        auto textureUAV = (rs_image_vk*)defalut_no_texture_UAV;

		destroyRsSampler(ctx, sampler);
		destroyRsBuffer(ctx, buffer);
        destroyRsBuffer(ctx, bufferUAV);
        destroyRsImage(ctx, texture);
        destroyRsImage(ctx, textureUAV);
    }

    void deinitVulkanBackEnd(rs_context_vk* ctx)
    {
        destroyRsSemaphore(ctx, ctx->semphoreToSignalPresent);
        destroyDefaultResources(ctx);

        if (ctx->computeQueue)
            vkQueueWaitIdle(ctx->computeQueue->queue);
        if(ctx->transferQueue)
            vkQueueWaitIdle(ctx->transferQueue->queue);

        vkQueueWaitIdle(ctx->graphicQueue->queue);

        if(ctx->presentQueue)
            vkQueueWaitIdle(ctx->presentQueue->queue);
        ctx->destroyer->clearAll(ctx);
        ctx->cmdBufferMgr->clearAll(ctx);
        delete ctx->cmdBufferMgr;
        delete ctx->destroyer;
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
        int iw = 0;
        int ih = 0;
        int il = 0;


        if (imageNum > 0) {
            iw = images[0]->width;
			ih = images[0]->height;
			il = images[0]->arrayLayers;
        }
        else if (depthStencil != nullptr) {
			iw = depthStencil->width;
			ih = depthStencil->height;
			il = depthStencil->arrayLayers;
        }
        else {
            assert(false);
            return nullptr;
        }

        for (int i = 0; i < imageNum; ++i) {
            const auto img = images[i];
            if (img->width != iw && img->height != ih && img->arrayLayers != il) {
                Log::error("Miss match width/height/array layers in render target.");
                return nullptr;
            }
        }

        std::vector<rs_image*> localimages;
        std::vector<rs_image_view*> views;
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
            views.push_back(img.defaultView);
            localimages.push_back((rs_image*) & img);
        }

        rs_rendertarget_vk* rt = new rs_rendertarget_vk;
        rt->m_views                 = views;
        rt->m_attachments           = localimages;
        rt->m_depthStencilAttachment = depthStencil;
        rt->m_dsView = depthStencil ? depthStencil->defaultView : nullptr;
        rt->rtPassHash = CalcRenderTargetPassHash(ctx, rt);
        return rt;
    }

	Render::Vulkan::rs_rendertarget_vk* createRsRenderTarget(rs_context_vk* ctx, rs_image_vk** images, ImageViewKey* imageViewKeys, int imageNum, bool havedepthLast)
	{
        rs_image_vk* dsImg = nullptr;
        if (havedepthLast) {
            dsImg = images[imageNum - 1];
        }
		int iw = -1;
		int ih = -1;
		int il = -1;

		for (int i = 0; i < imageNum; ++i) {
			const auto img = images[i];
            const auto& viewkey = imageViewKeys[i];
            if (viewkey.getMipCount() > 1) {
				Log::error("Mip count should be set to 1 in render target.");
				return nullptr;
			}
            int baseMip = (1 << viewkey.getBaseMip());
            uint32_t iw_new = std::max(1, int(images[i]->width  / baseMip));
			uint32_t ih_new = std::max(1, int(images[i]->height / baseMip));
            uint32_t il_new = viewkey.getLayerCount();
            if (iw == -1 || ih == -1 || il == -1) {
                iw = iw_new;
				ih = ih_new;
				il = viewkey.getLayerCount();
                continue;
            }

			if (iw_new != iw && ih_new != ih && il_new != il) {
				Log::error("Miss match width/height/array layers in render target.");
				return nullptr;
			}
		}

		std::vector<rs_image*> localimages;
		std::vector<rs_image_view*> localviews;
		int depthAttPos = -1;
        rs_image_view* dsView = nullptr;
        for (int i = 0; i < imageNum; ++i) {
            bool isDepth = (havedepthLast && i == imageNum - 1);
            auto& img = *(images[i]);
            if (!isDepth && !(img.usage & ImageUsage_ColorAttachment)) {
                assert(0 && "Image usage not match");
                return nullptr;
            }
            if (img.width != iw || img.height != ih || imageViewKeys->getLayerCount() != il) {
                assert(0 && "Attachment size not match");
                return nullptr;
            }
            const auto& view = getRsImageView(ctx, &img, imageViewKeys[i]);
            if (i == imageNum - 1 && havedepthLast) {
                dsView = view;
            }
            else {
                localviews.push_back(getRsImageView(ctx, &img, imageViewKeys[i]));
                localimages.push_back((rs_image*)&img);
            }
        }

		rs_rendertarget_vk* rt = new rs_rendertarget_vk;
		rt->m_views = localviews;
		rt->m_attachments = localimages;
        rt->m_depthStencilAttachment = dsImg;
        rt->m_dsView = dsView;
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
        VkBufferCreateFlags bufferCreateFlag = 0;
        auto toUseQueue = desc.queueType;
        //TODO:
        assert(Util::ContainsAll(context->graphicQueue->queueType, toUseQueue));
        uint32_t queueFamily = context->graphicQueue->familyIndex;
        VkBufferCreateInfo CI{};
        CI.        sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        CI. pNext = nullptr;
        CI.     flags = bufferCreateFlag;
        CI.     size = desc.byteSize;
        CI.     usage = toVkBufferUsageFlags(desc.bufUsage);
        CI.          sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        CI. queueFamilyIndexCount = 1;
        CI. pQueueFamilyIndices = &queueFamily;
        
        if (BufferDeviceAddressEnable) {
            CI.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        }

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
        if (!isRsBufferMappable(context, buffer)) {
            assert(false);
            return buffer->mappedPtr;
        }
        //User should notice the frame sync problem.
        //And we just pretend no GPU is using this buffer now.
        buffer->state = ResourceState::HostWrite;

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

        if (desc.type == ImageType::VCube || desc.type == ImageType::VCube_Array) {
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            ici.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        VmaAllocationCreateInfo ai{};
        ai.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        VmaAllocationInfo aif;
        vmaCreateImage(ctx->allocator, &ici, &ai, &image, &alloc, &aif);

		auto ret = new rs_image_vk;
		ret->native = image;
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

        auto defaultView = createRsImageView(ctx, ret, ret->type, (ret->usage), 0, desc.mipLevels, 0, desc.arrayLayers,UAVAccess::ReadOnly);
        ret->subresourceStates.resize(desc.mipLevels * desc.arrayLayers, ResourceState::Common);
        ret->subresourcePendingStates.resize(desc.mipLevels * desc.arrayLayers, ResourceState::Common);
        ret->defaultView = defaultView;
        return ret;
    }

    void destroyRsImage(rs_context_vk* context, rs_image_vk*& image, bool immediately)
    {
        if (!immediately) {
            context->destroyer->destroyImage(context->nextRenderFrame, image);
            return;
        }
        vmaDestroyImage(context->allocator, (VkImage)image->native, image->allocation);
        
		for (auto& view : image->imageViews) {
			destroyRsImageView(context, view);
		}
        image->imageViews.clear();
        delete image;
        image = 0;
    }

    size_t getRsImageSize(rs_image_vk* image)
    {
        return image->allocation->GetSize();
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
                return compileShader(context, *desc.compileDesc,desc.entryPoint);
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
        shader->rflInfo =std::move( reflectShader((uint32_t*)desc.shaderCode, desc.codeSizeByte, desc.stage) );
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
        ctx->destroyer->destroySemaphores(sem);
        auto semNative = (VkSemaphore*)sem->native;
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
        ctx->destroyer->destroyFences(fence);
        delete[] (VkFence*)(fence->native);
        delete fence;
        fence = 0;
    }

    void resetRsFence(rs_context_vk* ctx, rs_fence_vk* fence,int frameInFlight)
    {
        int fif = frameInFlight < 0 ? getWaitFif(ctx, fence->waitFlag) : frameInFlight;
        auto fenceNative = ((VkFence*)fence->native)[fif];
        vkResetFences(ctx->device, 1, &fenceNative);
    }

    void resetRsFence(rs_context_vk* ctx, rs_fence_vk* fence)
    {
        auto fenceNative = (VkFence*)fence->native;
        vkResetFences(ctx->device, fence->cnt, fenceNative);
    }

    void waitForRsFence(rs_context_vk* ctx, rs_fence_vk* fence, uint64_t timeout, int frameInFlight)
    {
        int fif = frameInFlight < 0 ? getWaitFif(ctx, fence->waitFlag) : frameInFlight;
        auto fenceNative = ((VkFence*)fence->native)[fif];
        auto res = vkWaitForFences(ctx->device, 1, &fenceNative, VK_TRUE, timeout);
        if (res != VK_SUCCESS) {
            Log::warn("wait for fence failed: " + std::to_string(res));
        }
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
        sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
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

			rs_image_vk* rsImage = new rs_image_vk;
			rsImage->native = img;
			rsImage->width = wWidth;
			rsImage->height = wHeight;
			rsImage->type = ImageType::V2D;
			rsImage->depth = 1;
			rsImage->mipLevels = 1;
			rsImage->usage = ImageUsage_PresentSrc | ImageUsage_ColorAttachment;
			rsImage->format = ToImageFormat(choosenFormat.format);
			rsImage->arrayLayers = 1;
            auto view = createRsImageView(context, rsImage, ImageType::V2D, ViewAspect::Color, 0, 1, 0, 1, UAVAccess::ReadOnly);
			rsImage->defaultView = view;
            rsImage->subresourceStates.resize(1, ResourceState::Common);
            rsImage->subresourcePendingStates.resize(1, ResourceState::Common);
			swapchainImages.push_back(rsImage);
        }
        context->hardwareFrameInFlight = swapchainImages.size();
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
        //Images are created by swapchain  
        auto& swapchainImages = context->swapchain->swapchainImgs;
        for (auto image : swapchainImages) {
		    image->defaultView = nullptr;
            for (auto& view : image->imageViews) {
				destroyRsImageView(context, view);
            }
            delete image;
        }
        swapchainImages.resize(0);
    }

	void cmdsetRenderTarget(rs_context_vk* ctx, rs_commandbuffer_vk* cmd, rs_rendertarget_vk* rt,const Rect2D& renderArea)
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

		auto rtSize = getRenderTargetSize(rt);
		int width = rtSize.width;
		int height = rtSize.height;
		int arrLayer = rtSize.arrayLayers;
		VkFramebuffer* frameBuffer = (VkFramebuffer*)&rt->native;
		VkRenderPass renderPass = (VkRenderPass)cmd->currentRenderPass->native;
        if (*frameBuffer == nullptr){
			std::lock_guard<std::mutex> lock(rt->mMutex);
            //Double check
			if (*frameBuffer == nullptr) {
				//Create one 
				std::vector<VkImageView> imageViews;
                imageViews.reserve(rt->m_attachments.size() + (rt->m_depthStencilAttachment != nullptr ? 1 : 0));
				for (const auto& i : rt->m_views) {
					imageViews.push_back((VkImageView)i->native);
				}
				if (rt->m_depthStencilAttachment) {
					imageViews.push_back((VkImageView)rt->m_dsView->native);
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
		}

        //Before begin render pass
        cmdTransitPendingResource(cmd, false);

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
        info.renderArea.extent.width     = (renderArea.r - renderArea.l) * width;
        info.renderArea.extent.height    = (renderArea.b - renderArea.t) * height;
        info.renderArea.offset.x         = renderArea.l * width;
		info.renderArea.offset.y         = renderArea.l * height;

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

    Render::rs_image_view* createRsImageViewInner(rs_context_vk* ctx, rs_image* image, ImageType viewType, VkImageAspectFlags aspect, uint16_t baseMip, uint16_t mipCnt, uint16_t baseLayer, uint16_t layerCnt) {
		rs_image_view* rsView = new rs_image_view;
		VkImageViewCreateInfo ivci{};
		ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ivci.image = (VkImage)image->native;
		ivci.viewType = toVkImageViewType(viewType);
		ivci.format = toVkFormat(image->format);
		ivci.components = { 
                VK_COMPONENT_SWIZZLE_IDENTITY ,
				VK_COMPONENT_SWIZZLE_IDENTITY ,
				VK_COMPONENT_SWIZZLE_IDENTITY ,
				VK_COMPONENT_SWIZZLE_IDENTITY
        };

		ivci.subresourceRange.aspectMask = (aspect);
		ivci.subresourceRange.baseMipLevel = baseMip;
		ivci.subresourceRange.levelCount = mipCnt;
		ivci.subresourceRange.baseArrayLayer = baseLayer;
		ivci.subresourceRange.layerCount = layerCnt;
        VK_CHECK(vkCreateImageView(ctx->device, &ivci, 0, (VkImageView*)&rsView->native), { assert(false);return {}; });
        return rsView;
    }

	Render::rs_image_view* createRsImageView(rs_context_vk* ctx, rs_image* image, ImageType viewType, ViewAspect aspect, uint16_t baseMip, uint16_t mipCnt, uint16_t baseLayer, uint16_t layerCnt, UAVAccess uav)
	{
        auto view = createRsImageViewInner(ctx, image, viewType, fromEngineAspecttoVkAspect(aspect), baseMip, mipCnt, baseLayer, layerCnt);
        view->image = image;
        view->viewKey = genViewKey(viewType, aspect, baseMip, mipCnt, baseLayer, layerCnt, uav);
		image->imageViews.emplace_back(view);
        return view;
    }

	Render::rs_image_view* createRsImageView(rs_context_vk* ctx, rs_image* image, ImageType viewType, uint32_t imageUsage, uint16_t baseMip, uint16_t mipCnt, uint16_t baseLayer, uint16_t layerCnt, UAVAccess uav)
	{

		ViewAspect aspect = ViewAspect::Color;
		if (imageUsage & ImageUsage_DepthStencilAttachment) {
			if (image->format == ImageFormat::D16_UNORM || image->format == ImageFormat::D32_SFLOAT) {
				aspect = ViewAspect::Depth;
			}
			else {
				aspect = ViewAspect::DepthAndStencil;
			}
		}
		auto view = createRsImageViewInner(ctx, image, viewType, fromEngineAspecttoVkAspect(aspect), baseMip, mipCnt, baseLayer, layerCnt);


        view->image = image;
        view->viewKey = genViewKey(image->type, aspect, baseMip, mipCnt, baseLayer, layerCnt, uav);
		image->imageViews.emplace_back(view);
		return view;
	}

    VkImageAspectFlags fromEngineAspecttoVkAspect(ViewAspect aspect)
	{
        switch (aspect)
        {
        case Render::ViewAspect::DepthAndStencil:
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			break;
        case Render::ViewAspect::Depth:
            return VK_IMAGE_ASPECT_DEPTH_BIT;
            break;
        case Render::ViewAspect::Stencil:
			return VK_IMAGE_ASPECT_STENCIL_BIT;
			break;
        case Render::ViewAspect::Color:
			return VK_IMAGE_ASPECT_COLOR_BIT;
			break;
        default:
            break;
        }
        return VK_IMAGE_ASPECT_COLOR_BIT;
	}

    VkImageAspectFlags toVkAspect(uint32_t usageFlags)
	{
        VkImageAspectFlags ret = VkImageAspectFlagBits::VK_IMAGE_ASPECT_NONE;
        if ( (usageFlags & uint32_t(ImageUsage_ColorAttachment)) || (usageFlags & uint32_t(ImageUsage_Sampled)) || (usageFlags & uint32_t(ImageUsage_Storage))) {
            ret = (ret | VK_IMAGE_ASPECT_COLOR_BIT);
        }
        if (usageFlags & uint32_t(ImageUsage_DepthStencilAttachment)) {
            ret = (ret | VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
        }
        return ret;
	}

	void destroyRsImageView(rs_context_vk* ctx,rs_image_view* view)
	{
        vkDestroyImageView(ctx->device, (VkImageView)view->native, 0);
        view->native = nullptr;
        view->viewKey.value = 0;
        delete view;
    }

	Render::rs_image_view* getRsImageView(rs_context_vk* ctx, rs_image* image, const ImageViewKey& viewKey)
	{
        for (auto& imageView : image->imageViews) {
            if (imageView->viewKey == viewKey) {
                return imageView;
            }
        }

        //Create:
        auto& viewBits = viewKey.bits;
        return createRsImageView(ctx, image, (ImageType)viewBits.viewType,
            (ViewAspect)viewBits.aspect,
            viewBits.baseMip, viewBits.mipCount,
            viewBits.baseLayer, viewBits.layerCount,(UAVAccess)viewBits.uavAccess);
	}

	void updateDrawData(rs_context_vk* context, uint64_t frame, rs_graphic_pipeline_vk* pipeline, rs_drawdata_vk* drawdata, rs_binding_pos pos,int subscript, rs_image_vk* vk,rs_image_view* view)
	{
		auto vkPos = toVkBindingPos(pos);
		auto fif = context->LogicFrameFif;
		auto [descriptorSet,setPack] = _findOrCreateDescripotrSet(context, frame, fif, pipeline, drawdata, vkPos.setIdx);
		//1. to see if binding is correct
		assert(vkPos.bindingIdx < setPack->bindingTracker.size());
        auto& slot  =setPack->bindingTracker[vkPos.bindingIdx];
        if (slot.type != UniformType::StorageImage &&
            slot.type != UniformType::Texture &&
            slot.type != UniformType::InputAttachment
            ) {
            Log::error("Binding error for type wrong.");
        }
        updateDrawData(context, slot, pos, subscript, view, 0);
    }

	void updateDrawData(rs_context_vk* context,rs_binding_slot& slot, rs_binding_pos pos, int subscript, void* base, uint32_t dyOffset, uint32_t bufferOffset, uint32_t bufferSize)
	{
        if (!base)return;
        if (dyOffset > 0) {
            assert(subscript == 0 && "This is not support");
        }
        //1. get real binding pos 
        auto vkPos = toVkBindingPos(pos);
        //2 update tracker's data and update dirty flag
        auto& binding = slot;
        //2.1 compare if resource is the same
        if (subscript >= binding.rsData.size()) {
            Log::error("Binding Error: subscript out of range.");
            return;
        }
        if (binding.rsData[subscript] == base) {
            if (binding.bufferSize != bufferSize || binding.bufferOffset != bufferOffset) {
                //This will cause set dirty.
                binding.fifDirtyFlag = 0xFFFF;
                binding.bufferOffset = bufferOffset;
                binding.bufferSize   = bufferSize;
            }

            //notice: If buffer binding still same ,then dynamic offset change is OK.
			binding.uboDyOffset = dyOffset;
			return;
        }
        else {
            binding.rsData[subscript] = base;
            binding.fifDirtyFlag = 0xFFFF;
            binding.bufferSize = bufferSize;
            binding.uboDyOffset = dyOffset;
        }
    }

	void flushRsBuffer(rs_context_vk* context, rs_buffer_vk* buffer, uint32_t size)
	{
        uint64_t flushSize = VK_WHOLE_SIZE;
        if (size <= buffer->byteSize) {
            flushSize = size;
        }
        if (buffer->mappedPtr) {
            vmaFlushAllocation(
                context->allocator, buffer->allocation, 0, flushSize
            );
        }


	}

	bool isBindlessEnabled()
	{
        return BindlessAvailable;
	}

	void cmdBindBindlessData(rs_context_vk* ctx, rs_commandbuffer_vk* cmd, rs_pipeline_layout_vk* pipelineLayout, rs_bindless_data_vk* bindlessData, QueueType bindPoint)
	{
        if (!isBindlessEnabled()) {
            return;
        }

        //Find set inside pipelineLayout
        uint32_t setIdx = INVALID_BINDING_POS;
        auto setLayout = bindlessData->descriptorSet->layout;
        for (auto& [set, layout] : pipelineLayout->setLayouts) {
            if (layout->bindingHash.layoutHash == setLayout->bindingHash.layoutHash) {
                assert(layout->native == setLayout->native);
                setIdx = set;
                break;
            }
        }

        if (setIdx == INVALID_BINDING_POS) {
            return;
        }

        bool noCacheState = (bindPoint & QueueType_Compute);

        if (!noCacheState) {
            if (cmd->bindedDescriptorSets[setIdx].set == bindlessData->descriptorSet->native) {
                return;
            }
            else {
                cmd->bindedDescriptorSets[setIdx].set = (VkDescriptorSet)bindlessData->descriptorSet->native;
            }
        }
#if 0
        //This somehow will lead to crash when renderdoc is open
        VkBindDescriptorSetsInfoKHR bindingInfo{VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO_KHR};
        bindingInfo.pNext           = nullptr;
        bindingInfo.layout          = (VkPipelineLayout)pipelineLayout->native;
        bindingInfo.firstSet        = setIdx;
        bindingInfo.descriptorSetCount = 1;
        bindingInfo.pDescriptorSets = (VkDescriptorSet*) (&bindlessData->descriptorSet->native);
        bindingInfo.dynamicOffsetCount = 0;
        bindingInfo.pDynamicOffsets = nullptr;
        bindingInfo.stageFlags = toVkShaderStageFlags(pipelineLayout->shaderStagesFlags);
        vkCmdBindDescriptorSets2KHR((VkCommandBuffer)cmd->native , &bindingInfo);
#else
		VkPipelineBindPoint point = pipelineLayout->shaderStagesFlags & (uint32_t)ShaderStage::Compute ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;
		vkCmdBindDescriptorSets((VkCommandBuffer)cmd->native, point, (VkPipelineLayout)pipelineLayout->native, setIdx, 1, (VkDescriptorSet*)(&bindlessData->descriptorSet->native), 0, nullptr);
#endif
	}

	uint64_t getRsBufferDeviceAddress(rs_context_vk* ctx, rs_buffer_vk* buffer)
	{
        if (!BufferDeviceAddressEnable) {
            return 0;
        }
        if (buffer->gpuAddress != 0)return buffer->gpuAddress;
		VkBufferDeviceAddressInfoKHR address_info{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR };
		address_info.buffer = (VkBuffer)buffer->native;
		buffer->gpuAddress = vkGetBufferDeviceAddressKHR(ctx->device, &address_info);
        return buffer->gpuAddress;
	}

	void beginFrameUnbindBindlessResource(rs_context_vk* ctx, rs_bindless_data_vk* bindlessData)
	{
        if (!isBindlessEnabled())return;
		std::vector<VkWriteDescriptorSet> writes{};
        std::vector<VkDescriptorImageInfo> imgInfos;
        auto fif = ctx->LogicFrameFif;
        imgInfos.reserve(bindlessData->pendingUnbindSampler[fif].size());
		VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		for (auto idx : bindlessData->pendingUnbindSampler[fif]) {
			bindlessData->samplersBinding.Free(idx);
			auto bindingIdx = toVkBindingPos(bindlessData->samplerBindlessPos).bindingIdx;
			write.dstSet = (VkDescriptorSet)bindlessData->descriptorSet->native;
			write.descriptorCount = 1;
			write.dstBinding = bindingIdx;
			write.dstArrayElement = idx;
			write.descriptorCount = 1;
			write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			VkDescriptorImageInfo imgInfo{};
			imgInfo.sampler = (VkSampler)defalut_no_sampler->native;
            imgInfos.push_back(imgInfo);
			write.pImageInfo = &imgInfos.back();
            writes.push_back(write);
        }
		vkUpdateDescriptorSets(ctx->device, writes.size(), writes.data(), 0, 0);
        writes.clear();
        imgInfos.clear();
        bindlessData->pendingUnbindSampler[fif].clear();

        imgInfos.reserve(bindlessData->pendingUnbindStorage[fif].size());
		for (auto idx : bindlessData->pendingUnbindStorage[fif]) {
            bindlessData->storageImagesBinding.Free(idx);
			auto bindingPos = bindlessData->storageBindlessPos ;
			auto bindingIdx = toVkBindingPos(bindingPos).bindingIdx;
            auto bindResource = defalut_no_texture_UAV->defaultView;
			VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			write.descriptorCount = 1;
			write.descriptorType = toVkDescriptorType(UniformType::StorageImage);
			write.dstSet = (VkDescriptorSet)bindlessData->descriptorSet->native;
			write.dstBinding = bindingIdx;
			write.dstArrayElement = idx;
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageView = (VkImageView)bindResource->native;
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			imgInfos.push_back(imageInfo);
			write.pImageInfo = &imgInfos.back();
			writes.push_back(write);
		}
		vkUpdateDescriptorSets(ctx->device, writes.size(), writes.data(), 0, 0);
		writes.clear();
		imgInfos.clear();
        bindlessData->pendingUnbindStorage[fif].clear();

		imgInfos.reserve(bindlessData->pendingUnbindTexture[fif].size());
		for (auto idx : bindlessData->pendingUnbindTexture[fif]) {
			bindlessData->texturesBinding.Free(idx);
			auto bindingPos = bindlessData->textureBindlessPos;
			auto bindingIdx = toVkBindingPos(bindingPos).bindingIdx;
			auto bindResource = defalut_no_texture->defaultView;
			VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			write.descriptorCount = 1;
			write.descriptorType = toVkDescriptorType(UniformType::Texture);
			write.dstSet = (VkDescriptorSet)bindlessData->descriptorSet->native;
			write.dstBinding = bindingIdx;
			write.dstArrayElement = idx;
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageView = (VkImageView)bindResource->native;
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imgInfos.push_back(imageInfo);
			write.pImageInfo = &imgInfos.back();
			writes.push_back(write);
		}
		vkUpdateDescriptorSets(ctx->device, writes.size(), writes.data(), 0, 0);
		writes.clear();
		imgInfos.clear();
		bindlessData->pendingUnbindTexture[fif].clear();
	}

	void bindlessDataMarkResource(rs_bindless_data_vk* bindlessData, rs_image_view* view, bool isUAV)
	{
        if (view->bindlessIndex == INVALID_BINDLESS_INDEX) {
            assert(false);
            return;
        }
        if (isUAV) {
            bindlessData->storageImagesBinding.ResourceStateMark(view->bindlessIndex);
        }
        else {
			bindlessData->texturesBinding.ResourceStateMark(view->bindlessIndex);
        }
	}

    void bindlessDataMarkResource(rs_bindless_data_vk* bindlessData, rs_buffer* buffer, bool isUAV)
    {
        if (isUAV) {
            bindlessData->mPendingBuffersUAV.push_back(buffer);
		}
        else {
            bindlessData->mPendingBuffersSRV.push_back(buffer);
        }
    }

	bool cmdClearImage(rs_commandbuffer_vk* cb, rs_image_view* view, const vec4& color)
	{
        if ((view->image->usage & ImageUsage_TransferDst) == 0) {
            return false;
        }

		//DO A STATE TRANSIT, IT'S A REQUIRE 
		Vulkan::transitionImageViewState(cb, view, ResourceState::TransferDst);


        
        auto resourceRange = toVkImageSubresourceRange(view->viewKey);

        auto currentResourceState = getViewState(view);
        auto mapping = getVulkanMapping(currentResourceState);

        if (view->image->usage & ImageUsage_DepthStencilAttachment) {
            VkClearDepthStencilValue clearCol{};
            clearCol.depth = color.r;
            clearCol.stencil = color.g;
			vkCmdClearDepthStencilImage(
				(VkCommandBuffer)cb->native,
				(VkImage)view->image->native,
				mapping.imageLayout,
				&clearCol,
				1, &resourceRange
			);
        }else{

			VkClearColorValue clearColor{};
			memcpy(clearColor.float32, &color, sizeof(vec4));
			//Remap?
			clearColor.int32[0] = int(color.r / 255.);
			clearColor.int32[1] = int(color.g / 255.);
			clearColor.int32[2] = int(color.b / 255.);
			clearColor.int32[3] = int(color.a / 255.);

			clearColor.uint32[0] = clearColor.int32[0];
			clearColor.uint32[1] = clearColor.int32[1];
			clearColor.uint32[2] = clearColor.int32[2];
			clearColor.uint32[3] = clearColor.int32[3];

			vkCmdClearColorImage(
				(VkCommandBuffer)cb->native,
				(VkImage)view->image->native,
				mapping.imageLayout,
				&clearColor,
				1, &resourceRange
			);
        }

        return true;
	}

	void cmdBlitImage(rs_commandbuffer_vk* cb, rs_context_vk* ctx, rs_image_vk* srcImage, rs_image_vk* dstImage, Filter filter)
	{
		VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

        if (srcImage->usage & ImageUsage_DepthStencilAttachment) {
            aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (srcImage->format == ImageFormat::D24_UNORM_S8_UINT || srcImage->format == ImageFormat::D32_SFLOAT_S8_UINT) {
                aspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }

		VkImageBlit blitRegion{};

		blitRegion.srcSubresource.aspectMask = aspectFlags;
		blitRegion.srcSubresource.mipLevel = 0;
		blitRegion.srcSubresource.baseArrayLayer = 0;
		blitRegion.srcSubresource.layerCount = 1;
		blitRegion.srcOffsets[0] = { 0, 0, 0 };
		blitRegion.srcOffsets[1] = { static_cast<int32_t>(srcImage->width), static_cast<int32_t>(srcImage->height), 1 };

		blitRegion.dstSubresource.aspectMask = aspectFlags;
		blitRegion.dstSubresource.mipLevel = 0;
		blitRegion.dstSubresource.baseArrayLayer = 0;
		blitRegion.dstSubresource.layerCount = 1;
		blitRegion.dstOffsets[0] = { 0, 0, 0 };
		blitRegion.dstOffsets[1] = { static_cast<int32_t>(dstImage->width), static_cast<int32_t>(dstImage->height), 1 };

		vkCmdBlitImage(
			(VkCommandBuffer)cb->native,
			(VkImage)srcImage->native,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			(VkImage)dstImage->native,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&blitRegion,
			toVkFilter(filter)
		);
	}

	void cmdFlushBuffer(rs_commandbuffer_vk* cb, rs_context_vk* context, rs_buffer_vk* bufferSrc)
	{
		VkBufferMemoryBarrier bufferBarrier{};
		bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT; 
		bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT; 
		bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bufferBarrier.buffer = (VkBuffer)bufferSrc->native;
		bufferBarrier.offset = 0;
		bufferBarrier.size = VK_WHOLE_SIZE;
		vkCmdPipelineBarrier(
			(VkCommandBuffer)cb->native,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
			0,
			0, nullptr,
			1, &bufferBarrier,                 
			0, nullptr
		);
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
        auto deviceFeatureEnable2 = getExtensionEnablePhysicalDevice2(context);
        auto vk13PhysicalDeviceFeature = getExtensionEnablePhysicalDeviceVk13(context);
        auto indexingFeature = getExtensionEnablePhysicalDeviceDescriptorIndexingFeatures(context);
        auto bufferDeviceAddressFeature = getExtensionEnablePhysicalDeviceBufferAddressFeatures(context);

		deviceFeatureEnable2.pNext = &indexingFeature;
        indexingFeature.pNext = &vk13PhysicalDeviceFeature;
        if (vk13PhysicalDeviceFeature.synchronization2 == true) {
            Synchronize2Enable = true;
        }

        if (indexingFeature.descriptorBindingPartiallyBound == true) {
            PartialBindingEnable = true;
        }

        {
            auto itor = extensionRequired.find(std::string(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME));
			BufferDeviceAddressEnable = (itor != extensionRequired.end()) && (bufferDeviceAddressFeature.bufferDeviceAddress == VK_TRUE); //!!! >= VK_13 this is a guaranteed support 
        }
        vk13PhysicalDeviceFeature.pNext = &bufferDeviceAddressFeature;

        {
                BindlessAvailable = (bool)(
                indexingFeature.runtimeDescriptorArray &&
                indexingFeature.descriptorBindingPartiallyBound &&

                indexingFeature.descriptorBindingSampledImageUpdateAfterBind &&
                indexingFeature.descriptorBindingStorageBufferUpdateAfterBind &&
                indexingFeature.descriptorBindingStorageImageUpdateAfterBind &&

                indexingFeature.shaderSampledImageArrayNonUniformIndexing &&
                indexingFeature.shaderStorageBufferArrayNonUniformIndexing &&

                indexingFeature.descriptorBindingVariableDescriptorCount &&
                BufferDeviceAddressEnable
                );
        }

        VkDeviceCreateInfo createInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        createInfo.pNext = &deviceFeatureEnable2;
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
        std::vector<const char*> extensionNames;
        for (const auto& ext : extensionRequired) {
            extensionNames.push_back(ext.c_str());
        }
        createInfo.enabledExtensionCount = extensionNames.size();
        createInfo.ppEnabledExtensionNames = extensionNames.data();
        createInfo. pEnabledFeatures = nullptr;
        
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
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
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

    std::set<std::string> getExtensionEnableDevice(rs_context_vk* context)
    {
        uint32_t extCount = 0;
        vkEnumerateDeviceExtensionProperties(context->physicalDevice,nullptr, &extCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extCount);
        vkEnumerateDeviceExtensionProperties(context->physicalDevice, nullptr, &extCount, availableExtensions.data());
        std::set<std::string> extensionSet;
        for (auto&& i : availableExtensions) {
            extensionSet.insert(std::string(i.extensionName));
        }

        std::set<std::string> requiredExtensionNames{};
            
        //swapchain extension
        requiredExtensionNames.insert(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        //For vkCmdBindDescriptorSets2KHR ---> promoted in vk14
        requiredExtensionNames.insert(VK_KHR_MAINTENANCE_6_EXTENSION_NAME);
        requiredExtensionNames.insert(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);

        std::erase_if(requiredExtensionNames, [&extensionSet](const std::string& in) {
            auto itor = extensionSet.find(in);
            if (itor == extensionSet.end())
            {
                Log::error(in + " Is not supported.");
                return true;
            }
            return false;
			});

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
        VkPhysicalDeviceFeatures2 referenceFeature{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        vkGetPhysicalDeviceFeatures2(context->physicalDevice, &referenceFeature);
        return referenceFeature;
    }

    VkPhysicalDeviceVulkan13Features getExtensionEnablePhysicalDeviceVk13(rs_context_vk* context) {
        VkPhysicalDeviceVulkan13Features features13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        VkPhysicalDeviceFeatures2 referenceFeature{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        referenceFeature.pNext = &features13;
        vkGetPhysicalDeviceFeatures2(context->physicalDevice, &referenceFeature);
        return features13;
    }

    VkPhysicalDeviceVulkan12Features getExtensionEnablePhysicalDeviceVk12(rs_context_vk* context)
    {
        VkPhysicalDeviceVulkan12Features features12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
        VkPhysicalDeviceFeatures2 referenceFeature{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        referenceFeature.pNext = &features12;
        vkGetPhysicalDeviceFeatures2(context->physicalDevice, &referenceFeature);
        return features12;
    }

    VkPhysicalDeviceDescriptorIndexingFeatures getExtensionEnablePhysicalDeviceDescriptorIndexingFeatures(rs_context_vk* context)
    {
        VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{};
        indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;

        VkPhysicalDeviceFeatures2 deviceFeatures2{};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &indexingFeatures;
        vkGetPhysicalDeviceFeatures2(context->physicalDevice, &deviceFeatures2);
        return indexingFeatures;
    }

    VkPhysicalDeviceBufferDeviceAddressFeatures getExtensionEnablePhysicalDeviceBufferAddressFeatures(rs_context_vk* context)
    {
        VkPhysicalDeviceBufferDeviceAddressFeatures BDAFeatures{};
        BDAFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;

        VkPhysicalDeviceFeatures2 deviceFeatures2{};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &BDAFeatures;
        vkGetPhysicalDeviceFeatures2(context->physicalDevice, &deviceFeatures2);
        return BDAFeatures;
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

    rs_drawdata_vk* createDrawData(rs_context_vk* context)
    {
        auto drawData = new rs_drawdata_vk;
        return drawData;
    }

    void clearDrawData(rs_context_vk* ctx, rs_drawdata_vk* drawdata)
    {

        for (auto& setPack : drawdata->DescriptorSets) {
            for (auto& set : setPack.descriptorSets) {
                if (set)
                {
                    ctx->descriptorSetMgr->ReturnDescriptorSet(ctx, set);
                }
            }
        }
    }

    void destroyDrawData(rs_context_vk* ctx, rs_drawdata_vk* drawdata)
    {
        ctx->destroyer->destroyDrawData(ctx->nextRenderFrame, drawdata);
    }
    
    std::pair<rs_descriptorSet_vk*, descriptor_set_pack*> _findOrCreateDescripotrSet(rs_context_vk* context,uint64_t frame, uint32_t fif,rs_graphic_pipeline_vk* pipeline, rs_drawdata_vk* drawdata, uint32_t vkSet) {
        auto& frameSets = drawdata->DescriptorSets;
        rs_descriptorSet_vk* targetSet = 0;
        descriptor_set_pack* tarSetPack = nullptr;
        bool isOneShot = drawdata->isOneShot;
        auto targetPipelineLayout = ((rs_pipeline_layout_vk*)pipeline->pipelineLayout);
        rs_descriptorset_layout_vk* targetSetLayout = nullptr;
        auto targetSetLayoutHash    = 0ull;
        for (auto& [setIdx, setLayout] : targetPipelineLayout->setLayouts) {
            if (setIdx == vkSet) {
                targetSetLayoutHash = setLayout->bindingHash.layoutHash;
                targetSetLayout = setLayout;
                break;
            }
        }

        //Not found
        if (targetSetLayoutHash == 0)return {nullptr,nullptr};
        

        for (auto& descriptorSetPack : drawdata->DescriptorSets) {
            //This may go wrong --- in some extremely wrong case:
            //layout hash may be the same? actually we can compare their VkDescriptorSetLayout handle, cause we ensure their only exists one handle for the same layout.
            if (descriptorSetPack.setlayout->bindingHash.layoutHash == targetSetLayoutHash) {
                assert(descriptorSetPack.setlayout->native == targetSetLayout->native && "Hash conflict.");
                tarSetPack = &descriptorSetPack;
                if (!isOneShot)
                {
                    //Not one shot situation, we will leave room for multi fif situation
                    if (descriptorSetPack.descriptorSets.size() < fif + 1) {
                        descriptorSetPack.descriptorSets.resize(fif + 1);
                        break;
                    }
                    else {
                        if (descriptorSetPack.descriptorSets[fif] == nullptr) {
                            break;
                        }
                        targetSet = descriptorSetPack.descriptorSets[fif];
                        break;
                    }
                }
                else {
					//One shot situation, just keep one 
                    if (descriptorSetPack.descriptorSets.size() < 1) {
                        descriptorSetPack.descriptorSets.resize(1, nullptr);
						break;
					}
                    else {
                        if (descriptorSetPack.descriptorSets[0] == nullptr) {
                            break;
                        }
                        else {
							targetSet = descriptorSetPack.descriptorSets[0];
							break;
                        }
                    }
                }
            }
        }

        bool bindingTackerNeedToCreate = false;
        if (!tarSetPack){
            bindingTackerNeedToCreate = true;
            descriptor_set_pack pack{};
            pack.setlayout = targetSetLayout;
            drawdata->DescriptorSets.emplace_back(std::move(pack));
            tarSetPack = &drawdata->DescriptorSets.back();
        }

        if (!targetSet) {
            targetSet = context->descriptorSetMgr->AllocateDescriptorSet(frame, context, pipeline, vkSet);
            if (!targetSet) {
                assert(0);
                return {nullptr,nullptr};
            }

            if (drawdata->isOneShot) {
                tarSetPack->descriptorSets.resize(1);
            }
            else {
                tarSetPack->descriptorSets.resize(context->maxFrameInFlight);
            }

            tarSetPack->descriptorSets[isOneShot ? 0 : fif] = targetSet;
			//Create binding tracker

                if (bindingTackerNeedToCreate) {
					if (!isOneShot) {
						const auto& bindingInfos = targetSet->layout->bindingHash
							.mDescriptors;

                    //create a vector with size maxof binding
                    uint16_t binding = 0;
                    for (auto& bindingItem : bindingInfos) {
                        binding = std::max(binding,uint16_t(toVkBindingPos(bindingItem.bindingPos).bindingIdx));
                    }
                    rs_binding_slot slot{};
                    slot.type = UniformType::Count;
                    tarSetPack->bindingTracker.resize(binding + 1, slot);
                    for (auto& bindingItem : bindingInfos) {
						rs_binding_slot slot{};
						slot.type = bindingItem.type;
						slot.rsData.resize(bindingItem.count);
						tarSetPack->bindingTracker[toVkBindingPos(bindingItem.bindingPos).bindingIdx] = (std::move(slot));
                    }
                }
            }
        }

        return {targetSet,tarSetPack};

    }

    void updateDrawData(rs_context_vk* context, uint64_t frame, rs_graphic_pipeline_vk* pipeline, rs_drawdata_vk* drawdata, rs_binding_pos pos, void* data, size_t size)
    {
		auto vkPos = toVkBindingPos(pos);
		auto fif = context->LogicFrameFif;
		auto [descriptorSet, setPack] = _findOrCreateDescripotrSet(context, frame, fif, pipeline, drawdata, vkPos.setIdx);
        if (setPack == nullptr)return;
		assert(vkPos.bindingIdx < setPack->bindingTracker.size());
		auto& slot = setPack->bindingTracker[vkPos.bindingIdx];

		if (slot.type != UniformType::UniformBuffer) {
			Log::error("Binding error for type wrong.");
			return;
		}
        UniformBufferObject* ubo = nullptr;
        uint32_t dyOffset = 0;
        if (slot.rsData[0] != nullptr) {
            //Has former ubo?
            ubo = (UniformBufferObject*)slot.rsData[0];
            //This will try to reallocate one from it and 
            auto pair = context->descriptorSetMgr->getDybuffer(frame, fif, context, data, size, QueueType_Graphics,(UniformBufferObject*) slot.rsData[0]);
            ubo = pair.first;
            dyOffset = pair.second;
        }
        else {
            auto pair = context->descriptorSetMgr->getDybuffer(frame, fif, context, data, size, QueueType_Graphics, nullptr);
            ubo = pair.first;
            dyOffset = pair.second;
        }
        //Store a ubo in it
        //No buffer offset was offered here.
		updateDrawData(context, slot, pos, 0, ubo, dyOffset,0,size);
    }

    void updateDrawData(rs_context_vk* context, uint64_t frame, rs_graphic_pipeline_vk* pipeline, rs_drawdata_vk* drawdata, rs_binding_pos pos, int subscript, rs_image_vk* image)
    {
		auto vkPos = toVkBindingPos(pos);
		auto fif = context->LogicFrameFif;
		auto [descriptorSet, setPack] = _findOrCreateDescripotrSet(context, frame, fif, pipeline, drawdata, vkPos.setIdx);

		assert(vkPos.bindingIdx < setPack->bindingTracker.size());
		auto& slot = setPack->bindingTracker[vkPos.bindingIdx];

		if (slot.type != UniformType::StorageImage && slot.type != UniformType::Texture && slot.type != UniformType::InputAttachment) {
			Log::error("Binding error for type wrong.");
			return;
		}

		updateDrawData(context, slot, pos, subscript, image->defaultView, 0);
    }

    void updateDrawData(rs_context_vk* context, uint64_t frame, rs_graphic_pipeline_vk* pipeline, rs_drawdata_vk* drawdata, rs_binding_pos pos, int subscript, rs_sampler_vk* vk)
    {
		auto vkPos = toVkBindingPos(pos);
		auto fif = context->LogicFrameFif;
		auto [descriptorSet, setPack] = _findOrCreateDescripotrSet(context, frame, fif, pipeline, drawdata, vkPos.setIdx);

		assert(vkPos.bindingIdx < setPack->bindingTracker.size());
		auto& slot = setPack->bindingTracker[vkPos.bindingIdx];

		if (slot.type != UniformType::Sampler) {
			Log::error("Binding error for type wrong.");
			return;
		}

		updateDrawData(context, slot, pos, subscript, vk, 0);
    }

    void updateDrawData(rs_context_vk* context, uint64_t frame, rs_graphic_pipeline_vk* pipeline, rs_drawdata_vk* drawdata, rs_binding_pos pos, int subscript, rs_buffer_vk* vk,uint32_t offset,uint32_t size)
    {
		auto vkPos = toVkBindingPos(pos);
		auto fif = context->LogicFrameFif;
		auto [descriptorSet, setPack] = _findOrCreateDescripotrSet(context, frame, fif, pipeline, drawdata, vkPos.setIdx);
        assert(offset + size <= vk->byteSize && "Buffer size limitation.");
		assert(vkPos.bindingIdx < setPack->bindingTracker.size());
		auto& slot = setPack->bindingTracker[vkPos.bindingIdx];

		if (slot.type != UniformType::ConstantBuffer && slot.type != UniformType::StorageBuffer) {
			Log::error("Binding error for type wrong.");
			return;
		}

		updateDrawData(context, slot, pos, subscript, vk, 0, offset,size);
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
		auto renderPass = (rs_renderpass_vk*)cb->currentRenderPass;
		auto renderTarget = cb->currentRenderTarget;
        
        for (int i = 0; i < renderTarget->m_attachments.size();++i) {
            changeViewStateNoBarrier(renderTarget->m_views[i], renderPass->finalStates[i]);
        }

        cb->currentRenderTarget = 0;
        cb->currentRenderPass = 0;
    }

    void cmdSetViewport(rs_commandbuffer_vk* cb,const Rect2D& rect, float minDepth, float maxDepth, uint32_t idx)
    {
        assert(cb->currentRenderPass != nullptr);
        VkViewport viewport{};
        auto& rt = cb->currentRenderTarget;
        auto rtSize = getRenderTargetSize(rt);
        int width = rtSize.width;
        int height = rtSize.height;
        int arrLayer = rtSize.arrayLayers;
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
		auto rtSize = getRenderTargetSize(rt);
		int width = rtSize.width;
		int height = rtSize.height;
		int arrLayer = rtSize.arrayLayers;
        
        VkRect2D scissor{};
        scissor.extent.width = (rect.r - rect.l) * width;
        scissor.extent.height = (rect.b - rect.t) * height;
        scissor.offset.x = rect.l * width;
        scissor.offset.y = rect.t * height;

        vkCmdSetScissor((VkCommandBuffer)cb->native, idx, 1, &scissor);
    }
    static void invalidCmdDescriptorCacheWhenChangePipelineLyout(rs_commandbuffer_vk* cmd, rs_pipeline_layout_vk* newLayout, VkPipelineBindPoint bindPoint) {
        //https://docs.vulkan.org/spec/latest/chapters/descriptorsets.html
        //SHIT RULE, SHIT API
        //If, additionally, the previously bound descriptor set for set N was bound using a pipeline layout not compatible for set N, then all bindings in sets numbered greater than N are disturbed.
        
        rs_pipeline_layout_vk* oldLayout = (rs_pipeline_layout_vk*)cmd->bindedPipelineLayout;
        if (!oldLayout) {
            cmd->bindedDescriptorSets = {};
            return;
        }
        uint32_t setSizeMax = std::min(newLayout->setLayouts.size(), oldLayout->setLayouts.size());

        uint32_t firstInvalidSet = cmd->bindedDescriptorSets.size();
        if (oldLayout->setLayouts.empty() && !newLayout->setLayouts.empty()) {
            firstInvalidSet = newLayout->setLayouts[0].first;
        }
        else if (!oldLayout->setLayouts.empty() && newLayout->setLayouts.empty()) {
            firstInvalidSet = oldLayout->setLayouts[0].first;
        }

        for (int i = 0;i < setSizeMax; ++i) {
            const auto& osl = oldLayout->setLayouts[i];
            const auto& nsl = newLayout->setLayouts[i];
            //Set Index not match or Set layout not match
            if (osl.first != nsl.first) {
                //Set Index not match
                firstInvalidSet = std::min(osl.first, nsl.first);
                break;
            }
            else if (osl.second != nsl.second) {
                //Set layout not match
                firstInvalidSet = osl.first;
                break;
            }
        }

        //Mark first not match set when all common layout are the same
        if (firstInvalidSet == cmd->bindedDescriptorSets.size()) {
            if (oldLayout->setLayouts.size() > newLayout->setLayouts.size()) {
                firstInvalidSet = oldLayout->setLayouts[setSizeMax].first;
            }
            else if (newLayout->setLayouts.size() > oldLayout->setLayouts.size()) {
                firstInvalidSet = newLayout->setLayouts[setSizeMax].first;
            }
        }

        //Invalid cache from the first not match setlayout
        for (int i = firstInvalidSet;i < cmd->bindedDescriptorSets.size();++i) {
            cmd->bindedDescriptorSets[i] = {};
        }

    }

    void cmdDispatch(rs_commandbuffer_vk* cb,rs_context_vk* ctx, rs_compute_pipeline_vk* pipeline, rs_drawdata_vk* drawData, rs_bindless_data_vk* bindless, uint32_t curFIF, int x, int y, int z)
    {
        if (pipeline == 0) {
            assert(0);
            return;
        }



        auto cmd = (VkCommandBuffer)cb->native;
        if (cb->bindedPipeline != pipeline->native) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, (VkPipeline)pipeline->native);
            cb->bindedPipeline = (VkPipeline)pipeline->native;
        }
        if (bindless) {
			cmdBindBindlessData(ctx, cb, (rs_pipeline_layout_vk*)pipeline->pipelineLayout, bindless,QueueType_Compute);
        }

		cmdBindDrawData(cb,ctx, (rs_pipeline_layout_vk*)pipeline->pipelineLayout, drawData, curFIF,QueueType_Compute);
        cmdTransitPendingResource(cb, true);
		vkCmdDispatch(cmd, x, y, z);
    }

    void cmdDrawIndexed(rs_commandbuffer_vk* cb, rs_context_vk* ctx, rs_graphic_pipeline_vk* pipeline, const RenderInfo& info, DrawDataArray drawDatas, uint32_t curFif, bool isInstanced)
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

        rs_pipeline_layout_vk* pipelineLayout = (rs_pipeline_layout_vk*)pipeline->pipelineLayout;
        if (pipeline->pipelineLayout != cb->bindedPipelineLayout) {
            invalidCmdDescriptorCacheWhenChangePipelineLyout(cb, pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS);
            cb->bindedPipelineLayout = pipelineLayout;
        }


        VkPipeline piplineVk = (VkPipeline)pipeline->native;
        if (cb->bindedPipeline != pipeline->native) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, piplineVk);
            cb->bindedPipeline = piplineVk;
        }
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
        
        for (int i = 0; i < drawDatas.size() - 1; ++i) {
			auto drawdata = drawDatas[i];
            if (drawdata) {
                cmdBindDrawData(cb, ctx, (rs_pipeline_layout_vk*)pipeline->pipelineLayout, drawdata, curFif, QueueType_Graphics);
            }
        }

        if(drawDatas.back()) {
            cmdBindBindlessData( ctx, cb, (rs_pipeline_layout_vk*)pipeline->pipelineLayout, (rs_bindless_data_vk*)drawDatas.back());
        }

        uint32_t instanceCnt = isInstanced ? info.instanceCount : 1;
        if (donotuseidxdraw) {
            vkCmdDraw(cmd, info.idxCount, instanceCnt, info.vtxoffset, 0);
        }
        else {
            vkCmdDrawIndexed(cmd, info.idxCount, instanceCnt, 0, info.vtxoffset, 0);
        }
    }

    void cmdDrawIndexed(rs_commandbuffer_vk* cb,rs_context_vk* ctx, rs_graphic_pipeline_vk* pipeline, const RenderInfo& info, rs_drawdata_vk* drawData, uint32_t curFif, bool isInstanced)
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

        rs_pipeline_layout_vk* pipelineLayout = (rs_pipeline_layout_vk*)pipeline->pipelineLayout;
        if (pipeline->pipelineLayout != cb->bindedPipelineLayout) {
            invalidCmdDescriptorCacheWhenChangePipelineLyout(cb, pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS);
            cb->bindedPipelineLayout = pipelineLayout;
        }

        auto cmd = (VkCommandBuffer)cb->native;
        VkPipeline piplineVk = (VkPipeline)pipeline->native;
        if (cb->bindedPipeline != pipeline->native) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, piplineVk);
            cb->bindedPipeline = piplineVk;
        }

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

        cmdBindDrawData(cb, ctx,(rs_pipeline_layout_vk*)pipeline->pipelineLayout, drawData, curFif);

        uint32_t instanceCnt = isInstanced ? info.instanceCount : 1;
        if (donotuseidxdraw) {
            vkCmdDraw(cmd, info.idxCount, instanceCnt, info.vtxoffset,0);
        }
        else {
            vkCmdDrawIndexed(cmd, info.idxCount, instanceCnt, 0, info.vtxoffset, 0);
        }
    }

    inline void descriptorSetWriteArray(rs_context_vk* context, VkDescriptorSet set, uint32_t binding, rs_binding_slot& slot) {
        assert(slot.rsData.size() > 1);
		VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet writeSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        writeSet.dstSet = set;
        writeSet.dstBinding = binding;
        writeSet.dstArrayElement = 0;
        writeSet.descriptorCount = static_cast<uint32_t>(slot.rsData.size());
        writeSet.descriptorType = toVkDescriptorType(slot.type);
        std::vector<void*> bindingData;
        bindingData.reserve(slot.rsData.size());
        bool enableNullBinding = PartialBindingEnable;
        switch (slot.type)
        {
        case UniformType::UniformBuffer:
        {
            assert(false && "Error this should not happen");
            return;
        }
        case UniformType::ConstantBuffer:
        case UniformType::StorageBuffer:
        {
            std::vector<VkDescriptorBufferInfo> bufferInfos;
            bufferInfos.reserve(slot.rsData.size());
			rs_buffer_vk* defaultBuffer = (rs_buffer_vk*)defalut_no_buffer;
            if (slot.type == UniformType::StorageBuffer) {
                defaultBuffer = (rs_buffer_vk*)defalut_no_buffer_UAV;
            }
            for (size_t i = 0; i < slot.rsData.size(); ++i) {
                VkDescriptorBufferInfo info{};
                //No buffer offset/size for array binding.
                if (slot.rsData[i] == nullptr) {
                    if (!enableNullBinding) {
                        slot.fifDirtyFlag = 0xFFFF; //Set to dirty all;
                        slot.rsData[i] = defaultBuffer;
                    }
                }
                info.buffer = (VkBuffer)(((rs_base*)slot.rsData[i])->native);
                info.offset = 0;
                info.range  = VK_WHOLE_SIZE;
                bufferInfos.push_back(info);
            }
            writeSet.pBufferInfo = bufferInfos.data();
            vkUpdateDescriptorSets(context->device, 1, &writeSet, 0, nullptr);
            break;
        }
        case UniformType::StorageImage:
        case UniformType::Texture:
        case UniformType::InputAttachment:
        {
            std::vector<VkDescriptorImageInfo> imageInfos;
            imageInfos.reserve(slot.rsData.size());
            VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			rs_image_vk* defaultImage = (rs_image_vk*)defalut_no_texture;
            if (slot.type == UniformType::StorageImage) {
                layout = VK_IMAGE_LAYOUT_GENERAL;
                defaultImage = (rs_image_vk*)defalut_no_texture_UAV;
            }

            for (size_t i = 0; i < slot.rsData.size(); ++i) {
                VkDescriptorImageInfo info{};
                info.imageLayout = layout;
                if (slot.rsData[i] == nullptr) {
                    if (!enableNullBinding)
                    {
                        slot.fifDirtyFlag = 0xFFFF; //Set to dirty all;
                        slot.rsData[i] = &(defaultImage->defaultView);
                    }
                    else {
                        info.imageLayout =(VkImageLayout) 0;
                    }
                }
                info.imageView = (VkImageView)((rs_base*)slot.rsData[i])->native;
                imageInfos.push_back(info);
            }
            writeSet.pImageInfo = imageInfos.data();
            vkUpdateDescriptorSets(context->device, 1, &writeSet, 0, nullptr);
            break;
        }
        case UniformType::Sampler:
        {
            std::vector<VkDescriptorImageInfo> imageInfos;
            for (size_t i = 0; i < slot.rsData.size(); ++i) {
                VkDescriptorImageInfo info{};
                if (slot.rsData[i] == nullptr) {
                    if (!enableNullBinding)
                    {
                        slot.fifDirtyFlag = 0xFFFF; //Set to dirty all;
                        slot.rsData[i] = defalut_no_sampler;
                    }
                    else {
                        info.imageLayout = (VkImageLayout)0;
                    }
                }

				info.sampler = (VkSampler)((rs_base*)slot.rsData[i])->native;
                imageInfos.push_back(info);
            }
            writeSet.pImageInfo = imageInfos.data();
            vkUpdateDescriptorSets(context->device, 1, &writeSet, 0, nullptr);
            break;
        }
        default:
            assert(false && "Unsupported UniformType in Array Write");
            return;
        }

    }
    inline void collectDyoffset(rs_binding_slot& slot, DyOffsetArray& dynamicOffset, int& offsetCnt) {
    
        if (offsetCnt >= dynamicOffset.size()) {
            Log::warn("Binding too much dyoffsets on descriptor! Clamped");
            return;
        }
        if (slot.type == UniformType::UniformBuffer) {
			dynamicOffset[offsetCnt++] = slot.uboDyOffset;
        }
    }

    inline void descriptorSetWrite(rs_context_vk* context, VkDescriptorSet set, uint32_t binding, rs_binding_slot& slot, DyOffsetArray& dynamicOffset, int& offsetCnt) {
        VkDevice device = context->device;
        VkWriteDescriptorSet writeSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        writeSet.dstSet = set;
        writeSet.dstBinding = binding;
        writeSet.dstArrayElement = 0;
        writeSet.descriptorCount = 1;
        writeSet.descriptorType = toVkDescriptorType(slot.type);

        bool isNull = slot.rsData[0] == nullptr;
        bool enableNullBinding = false;//PartialBindingEnable;

        switch (slot.type)
        {
            //Buffer Like
        case UniformType::UniformBuffer:
        {
            VkDescriptorBufferInfo bufferInfo{};
            if (isNull) {
                slot.fifDirtyFlag = 0xFFFF; //Set to dirty all;
                auto curFrameDyBufferDefault = context->descriptorSetMgr->getCurFrameDefaultUBO();
                bufferInfo.buffer = (VkBuffer)curFrameDyBufferDefault.first->mBuffer->native;
                bufferInfo.range = 8;
                bufferInfo.offset = 0;
                dynamicOffset[offsetCnt++] = curFrameDyBufferDefault.second;
            }
            else {
                //Store a ubo object in it
                bufferInfo.buffer = (VkBuffer)((UniformBufferObject*)slot.rsData[0])->mBuffer->native;
                bufferInfo.range = slot.bufferSize;
                bufferInfo.offset = slot.bufferOffset;
                dynamicOffset[offsetCnt++] = slot.uboDyOffset;
            }
            writeSet.pBufferInfo = &bufferInfo;
            vkUpdateDescriptorSets(device, 1, &writeSet, 0, nullptr);
            break;
        }
        case UniformType::ConstantBuffer:
        case UniformType::StorageBuffer:
        {
            VkDescriptorBufferInfo bufferInfo{};
            if (isNull) {
                rs_buffer_vk* defaultBuf = (slot.type == UniformType::StorageBuffer)
                    ? (rs_buffer_vk*)defalut_no_buffer_UAV
                    : (rs_buffer_vk*)defalut_no_buffer;
                bufferInfo.buffer = (VkBuffer)defaultBuf->native;
                slot.fifDirtyFlag = 0xFFFF;
            }
            else {
                bufferInfo.buffer = (VkBuffer)((rs_base*)slot.rsData[0])->native;
            }

            bufferInfo.offset = slot.bufferOffset;
            bufferInfo.range = (slot.bufferSize == 0) ? VK_WHOLE_SIZE : slot.bufferSize;

            writeSet.pBufferInfo = &bufferInfo;
            vkUpdateDescriptorSets(device, 1, &writeSet, 0, nullptr);
            break;
        }

        //Image like
        case UniformType::Texture:
        case UniformType::InputAttachment:
        case UniformType::StorageImage:
        {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = (slot.type == UniformType::StorageImage)
                ? VK_IMAGE_LAYOUT_GENERAL
                : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            if (isNull) {
                if (!enableNullBinding) {
                    rs_image_vk* defaultImg = (slot.type == UniformType::StorageImage)
                        ? (rs_image_vk*)defalut_no_texture_UAV
                        : (rs_image_vk*)defalut_no_texture;
                    imageInfo.imageView = (VkImageView)defaultImg->defaultView->native;
                    slot.fifDirtyFlag = 0xFFFF;
                }
                else {
                    return;
                }
            }
            else {
                imageInfo.imageView = (VkImageView)((rs_base*)slot.rsData[0])->native;
            }

            writeSet.pImageInfo = &imageInfo;
            vkUpdateDescriptorSets(device, 1, &writeSet, 0, nullptr);
            break;
        }

        //Sampler
        case UniformType::Sampler:
        {
            VkDescriptorImageInfo samplerInfo{};
            if (isNull) {
                samplerInfo.sampler = (VkSampler)defalut_no_sampler->native;
                slot.fifDirtyFlag = 0xFFFF;
            }
            else {
                samplerInfo.sampler = (VkSampler)((rs_base*)slot.rsData[0])->native;
            }

            writeSet.pImageInfo = &samplerInfo;
            vkUpdateDescriptorSets(device, 1, &writeSet, 0, nullptr);
            break;
        }
        default:
            assert(false && "Unsupported UniformType");
            break;
        }
    }

    void cmdBindDrawData(rs_commandbuffer_vk* cb, rs_context_vk* ctx, rs_pipeline_layout_vk* layout, rs_drawdata_vk* drawData, uint32_t curFif, QueueType bindPoint)
    {
        auto& descriptorPacks = ((rs_drawdata_vk*)drawData)->DescriptorSets;

        //Do real write in here
        for (auto& descriptorPack : descriptorPacks) {
            DyOffsetArray dynamics{};
            int dyNum = 0;

            auto setLayout = descriptorPack.setlayout;
            uint32_t setIdx = INVALID_BINDING_POS;
            for (auto& [set, layout] : layout->setLayouts) {
                if (layout->bindingHash.layoutHash == setLayout->bindingHash.layoutHash) {
                    assert(layout->native == setLayout->native);
                    setIdx = set;
                    break;
                }
            }
            
			//if pipeline doesnt have this set, just skip it.
			if (setIdx == INVALID_BINDING_POS)continue;

            //Avoid multi bind after set was bind to cmd.
            bool needUpadte = false;
            if (descriptorPack.setUpdatedFif == curFif) {
                needUpadte = false;
            }
            else {
                needUpadte = true;
                descriptorPack.setUpdatedFif = curFif;
            }

            uint32_t setFifToUse = curFif;
            if (drawData->isOneShot) {
                setFifToUse = 0;
            }
            VkDescriptorSet set = (VkDescriptorSet)descriptorPack.descriptorSets[setFifToUse]->native;
            if (needUpadte) {
                int bindingIdx = 0;
                for (auto& bindingSlot : descriptorPack.bindingTracker) {
                    int curBindingIdx = bindingIdx;
                    bindingIdx++;
                    auto type = bindingSlot.type;
                    if (type == UniformType::Count) {
                        //Not valid
                        continue;
                    }
                    //Is set dirty? 
                    if ((bindingSlot.fifDirtyFlag & (1u << curFif)) != 0) {
                        //Clear cur frame dirty flag.
                        bindingSlot.fifDirtyFlag &= ~(1u << curFif);
                        //Binding to descriptor by array type.
                        if (bindingSlot.rsData.size() > 1) {
                            descriptorSetWriteArray(ctx, set, curBindingIdx, bindingSlot);
                        }
                        else {
                            descriptorSetWrite(ctx, set, curBindingIdx, bindingSlot, dynamics, dyNum);
                        }

                    }
                    else {
                        collectDyoffset(bindingSlot, dynamics, dyNum);
                        
                    }
                }
            }
            else {
                for (auto& bindingSlot : descriptorPack.bindingTracker) {
					collectDyoffset(bindingSlot, dynamics, dyNum);
                }
            }


            if (setIdx >= cb->bindedDescriptorSets.max_size()) {
                Log::warn("Binding too much descriptors cannot be cached");
            }
            else {
                bool noCacheState = (bindPoint & QueueType_Compute) ? true : false;
                //For binding point compute, 
                //We need to cache another states
                if (!noCacheState) {
                    if (cb->bindedDescriptorSets[setIdx].set == set) {
                        bool canSkipThisBinding = true;
                        for (int i = 0;i < dyNum;++i) {
                            if (cb->bindedDescriptorSets[setIdx].offsetArray[i] != dynamics[i]) {
                                canSkipThisBinding = false;
                                break;
                            }
                        }
                        if (canSkipThisBinding) {
                            continue;
                        }
                    }
                }

            }

            //Cache
            cb->bindedDescriptorSets[setIdx].set            = set;
            cb->bindedDescriptorSets[setIdx].offsetArray    = dynamics;

            VkPipelineBindPoint point = (bindPoint & QueueType_Graphics ? VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS : VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE);
            vkCmdBindDescriptorSets((VkCommandBuffer)cb->native, point, (VkPipelineLayout)layout->native, setIdx, 1, &set, dyNum, dynamics.data());

        }
    }

    void cmdCopyBufferToBuffer(rs_commandbuffer_vk* cb, rs_context_vk* context, rs_buffer_vk* bufferSrc, rs_buffer_vk* bufferDst, uint64_t size, uint64_t srcOffset, uint64_t dstOffset)
    {
        cb->hasCommands = true;
        auto cmd = (VkCommandBuffer)cb->native;

        VkCopyBufferInfo2 cpInfo{ VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2 };
        cpInfo.srcBuffer = (VkBuffer)bufferSrc->native;
        cpInfo.dstBuffer = (VkBuffer)bufferDst->native;
        cpInfo.regionCount = 1;

        VkBufferCopy2 bufferCopy{ VK_STRUCTURE_TYPE_BUFFER_COPY_2 };
        bufferCopy.srcOffset = srcOffset;
        bufferCopy.dstOffset = dstOffset;
        bufferCopy.size = size;
        cpInfo.pRegions = &bufferCopy;

        vkCmdCopyBuffer2(cmd, &cpInfo);
    }

    inline void markPendingState(rs_buffer_vk* buffer, ResourceState targetState, std::vector<void*>& bucket)
    {
        if (buffer == nullptr) return;

        if (buffer->pendingState != targetState) {
            buffer->pendingState = targetState;  
            bucket.push_back((void*)buffer);   
        }
    }

    inline void markPendingState(rs_image_view* view, ResourceState targetState, std::vector<void*>& bucket)
    {
        if (view == nullptr || view->image == nullptr) return;

        rs_image_vk* image = (rs_image_vk*)view->image;
        auto& key = view->viewKey;

        uint32_t actualMips = (key.getMipCount() == VK_REMAINING_MIP_LEVELS) ? image->mipLevels - key.getBaseMip() : key.getMipCount();
        uint32_t actualLayers = (key.getLayerCount() == VK_REMAINING_ARRAY_LAYERS) ? image->arrayLayers - key.getBaseLayer() : key.getLayerCount();

        bool needsTransition = false;

        for (uint32_t layer = key.getBaseLayer(); layer < key.getBaseLayer() + actualLayers; ++layer) {
            for (uint32_t mip = key.getBaseMip(); mip < key.getBaseMip() + actualMips; ++mip) {

                uint32_t flatIndex = layer * image->mipLevels + mip;

                if (image->subresourcePendingStates[flatIndex] != targetState) {
                    image->subresourcePendingStates[flatIndex] = targetState;
                    needsTransition = true;
                }
            }
        }

        if (needsTransition) {
            bucket.push_back((void*)view);
        }
    }

    void cmdCollectDrawDataStateToTransit(rs_commandbuffer_vk* cb, rs_bindless_data_vk* drawData, PipelineType pipelineType, uint32_t curFif)
    {
        if (cb->resourceToBeTransit.size() < (int)UniformType::Count) {
            cb->resourceToBeTransit.resize((int)UniformType::Count);
        }

        bool isCompute = (pipelineType == PipelineType::Compute);
        //1. texture
        auto& bucketSRV = cb->resourceToBeTransit[(int)UniformType::Texture];
        for (auto denseIndex : drawData->texturesBinding.denseIndexToTransit) {
            markPendingState((rs_image_view*)drawData->texturesBinding.denseData[denseIndex].resource,
                isCompute ? Render::ResourceState::ComputeShaderResource : ResourceState::ShaderResource, bucketSRV);
        }
        drawData->texturesBinding.ClearResourceStateMarked();

		auto& bucketUAV = cb->resourceToBeTransit[(int)UniformType::StorageImage];
		for (auto denseIndex : drawData->storageImagesBinding.denseIndexToTransit) {
			markPendingState((rs_image_view*)drawData->storageImagesBinding.denseData[denseIndex].resource,
                isCompute ? Render::ResourceState::ComputeUnorderedAccess : ResourceState::UnorderedAccess, bucketUAV);
		}
        drawData->storageImagesBinding.ClearResourceStateMarked();

		auto& bucketBufferSRV = cb->resourceToBeTransit[(int)UniformType::ConstantBuffer];
		for (auto buffer : drawData->mPendingBuffersSRV) {
			markPendingState((rs_buffer_vk*)buffer,
                isCompute ? Render::ResourceState::ComputeShaderResource : ResourceState::ShaderResource, bucketBufferSRV);
		}
        drawData->mPendingBuffersSRV.clear();

		auto& bucketBufferUAV = cb->resourceToBeTransit[(int)UniformType::StorageBuffer];
		for (auto buffer : drawData->mPendingBuffersUAV) {
			markPendingState((rs_buffer_vk*)buffer,
                isCompute ? Render::ResourceState::ComputeUnorderedAccess : ResourceState::UnorderedAccess, bucketBufferUAV);
		}
        drawData->mPendingBuffersUAV.clear();

    }

    void cmdCollectDrawDataStateToTransit(rs_commandbuffer_vk* cb, rs_drawdata_vk* drawData, PipelineType pipelineType, uint32_t curFif)
    {
        if (cb->resourceToBeTransit.size() < (int)UniformType::Count) {
            cb->resourceToBeTransit.resize((int)UniformType::Count);
        }

        bool isCompute = (pipelineType == PipelineType::Compute);

        for (auto& setPack : drawData->DescriptorSets) {
            for (auto& bindingSlot : setPack.bindingTracker) {
                if (bindingSlot.type == UniformType::Count || bindingSlot.type == UniformType::Sampler) {
                    continue;
                }

                auto& bucket = cb->resourceToBeTransit[int(bindingSlot.type)];

                ResourceState targetState;
                switch (bindingSlot.type) {
                case UniformType::ConstantBuffer:
                case UniformType::UniformBuffer:
                case UniformType::Texture:
                case UniformType::InputAttachment:
                    targetState = isCompute ? ResourceState::ComputeShaderResource : ResourceState::ShaderResource;
                    break;
                case UniformType::StorageBuffer:
                case UniformType::StorageImage:
                    targetState = isCompute ? ResourceState::ComputeUnorderedAccess : ResourceState::UnorderedAccess;
                    break;
                default:
                    continue;
                }

                if (bindingSlot.type == UniformType::Texture ||
                    bindingSlot.type == UniformType::StorageImage ||
                    bindingSlot.type == UniformType::InputAttachment)
                {
                    for (int i = 0; i < bindingSlot.rsData.size(); ++i) {
                        markPendingState((rs_image_view*)bindingSlot.rsData[i], targetState, bucket);
                    }
                }
                else if (bindingSlot.type == UniformType::UniformBuffer)
                {
                    //UBO is a little different...
                    for (int i = 0; i < bindingSlot.rsData.size(); ++i) {
                        if (bindingSlot.rsData[i] == nullptr) continue;
                        UniformBufferObject* ubo = (UniformBufferObject*)bindingSlot.rsData[i];
                        markPendingState((rs_buffer_vk*)ubo->mBuffer, targetState, bucket);
                    }
                }
                else
                {
                    for (int i = 0; i < bindingSlot.rsData.size(); ++i) {
                        markPendingState((rs_buffer_vk*)bindingSlot.rsData[i], targetState, bucket);
                    }
                }
            }
        }
    }

    void cmdTransitPendingResource(rs_commandbuffer_vk* cb, bool compute)
    {
        for (int i = 0; i < cb->resourceToBeTransit.size(); ++i) {
            auto& bucket = cb->resourceToBeTransit[i];
            UniformType type = (UniformType)i;
            switch (type) {
            case UniformType::ConstantBuffer:
            case UniformType::UniformBuffer:
            {
                transitionBufferStateBatch(cb, bucket.begin(), bucket.end(), compute ? ResourceState::ComputeShaderResource : ResourceState::ShaderResource);
                break;
            }
            case UniformType::StorageBuffer:
            {
                transitionBufferStateBatch(cb, bucket.begin(), bucket.end(), compute ? ResourceState::ComputeUnorderedAccess : ResourceState::UnorderedAccess);
                break;
            }
            case UniformType::Texture:
            case UniformType::InputAttachment:
            {
                transitionImageStateBatch(cb, bucket.begin(), bucket.end(), compute ? ResourceState::ComputeShaderResource : ResourceState::ShaderResource);
                break;
            }
            case UniformType::StorageImage:
            {
                transitionImageStateBatch(cb, bucket.begin(), bucket.end(), compute ? ResourceState::ComputeUnorderedAccess : ResourceState::UnorderedAccess);
                break;
            }
            default:
                break;
            }
            bucket.clear();
        }
    }
    void cmdSubmitCmdBuffer(rs_context_vk* ctx, rs_commandbuffer_vk* cb, QueueType queue, std::vector<rs_semaphore*> imageAvailableWaitSemaphores, std::vector<rs_semaphore*> renderFinishSignalSemphores, rs_fence_vk* fence)
    {

		VkPipelineStageFlags toWaitFlag;
		switch (queue)
		{
		case Render::QueueType_Graphics:
			toWaitFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			break;
		case Render::QueueType_Compute:
			toWaitFlag = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;
		case Render::QueueType_Transfer:
			toWaitFlag = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case Render::QueueType_Present:
		default:
			toWaitFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			break;
		}

        auto curFif = ctx->RenderFrameFif;
		std::vector<VkSemaphore> ntvwaitSemaphores, ntvsignalSemaphores;
        std::vector< VkPipelineStageFlags> waitStageFlags;

        for (auto&& sem : imageAvailableWaitSemaphores) {
            auto mapping = getVulkanMapping(sem->waitResourceState);
            auto waitFlag = sem->waitFlag;
            uint32_t fif = getWaitFif(ctx,waitFlag);
            ntvwaitSemaphores.push_back( ((VkSemaphore*)sem->native)[fif]);
            waitStageFlags.push_back(mapping.stageMask | toWaitFlag);
        }
        for (auto&& sem : renderFinishSignalSemphores) {
			auto waitFlag = sem->waitFlag;
			uint32_t fif = getWaitFif(ctx, waitFlag);
            ntvsignalSemaphores.push_back(((VkSemaphore*)sem->native)[fif]);
        }




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
            auto fif = getWaitFif(ctx, fence->waitFlag);
            fencevk = ((VkFence*)fence->native)[fif];
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
        //Reset binded descriptors
        cb->bindedDescriptorSets = {};
        cb->bindedPipeline = VK_NULL_HANDLE;
        cb->bindedPipelineLayout = VK_NULL_HANDLE;
        vkBeginCommandBuffer((VkCommandBuffer)cb->native, &beginCi);
    }

    void cmdEndRecord(rs_commandbuffer_vk* cb)
    {
        vkEndCommandBuffer((VkCommandBuffer)cb->native);
    }

    void cmdSubmitOneShotAndWait(rs_context_vk* ctx, rs_commandbuffer_vk* cb)
    {
        auto fence = createRsFence(ctx);
        cmdSubmitCmdBuffer(ctx, cb, QueueType_Graphics, {}, {}, fence);
        waitForRsFence(ctx, fence,-1,ctx->RenderFrameFif);
        destroyRsFence(ctx,fence);
    }
//TODO: configable set size? this can be done by compile shader with given macro.....
#include "Renderer/GPUShared/BindlessGlobalDefShared.h"
    rs_bindless_data_vk* createBindlessData(rs_context_vk* ctx, rs_pipeline* pipeline, int setIdx)
    {
        rs_bindless_data_vk* bindlessData = new rs_bindless_data_vk(MAX_TEXTURE_BINDLESS, MAX_SAMPLER_BINDLESS, MAX_STORAGEIMAGE_BINDLESS);
        bindlessData->pendingUnbindSampler.resize(ctx->maxFrameInFlight);
        bindlessData->pendingUnbindStorage.resize(ctx->maxFrameInFlight);
        bindlessData->pendingUnbindTexture.resize(ctx->maxFrameInFlight);
        auto descriptorManager = ctx->descriptorSetMgr;
        bindlessData->descriptorSet = descriptorManager->AllocateDescriptorSetFromDedicatePool(ctx->nextRenderFrame, ctx, pipeline, setIdx);
        return bindlessData;
    }


	void destroyBindlessData(rs_context_vk* ctx, rs_bindless_data_vk* data)
	{
        ctx->destroyer->destroyBindlessData(ctx->nextRenderFrame, data);
	}

    static BindlessSlot GetBindlessSlot(rs_resource* data,uint64_t lastUsedFrame,UniformType type) {
        BindlessSlot slot{};
        slot.resource = data;
        slot.lastUsedFrame = lastUsedFrame;
        slot.type = type;
        return slot;
    }

    uint64_t updateBindlessData(rs_context_vk* ctx, rs_bindless_data_vk* bindlessData, rs_buffer_vk* buffer, bool uav)
    {
        if (!buffer) {
            return INVALID_BINDLESS_INDEX;
        }

        return getRsBufferDeviceAddress(ctx, buffer);

    }

    uint32_t updateBindlessImage(rs_context_vk* ctx, rs_bindless_data_vk* bindlessData, rs_image_view* view, bool uav)
    {

        if (view && view->bindlessIndex != INVALID_BINDLESS_INDEX) {
            view->bindingRef.fetch_add(1);
            return view->bindlessIndex;
        }

        UniformType type = uav ? UniformType::StorageImage : UniformType::Texture;
        auto textureSlot = GetBindlessSlot(view, ctx->nextRenderFrame, type);
        auto bindingTable = uav ? &bindlessData->storageImagesBinding : &bindlessData->texturesBinding;
        auto handleIndex = bindingTable->Allocate(ctx->nextRenderFrame, view);
        auto bindingPos = uav ? bindlessData->storageBindlessPos : bindlessData->textureBindlessPos;
        if (handleIndex == INVALID_BINDLESS_INDEX) {
            assert(false);
            //FIX ME 
        }
		view->bindingRef.fetch_add(1);
		view->bindlessIndex = handleIndex;

        auto bindingIdx = toVkBindingPos(bindingPos).bindingIdx;

        VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.descriptorCount = 1;
        write.descriptorType = toVkDescriptorType(type);
        write.dstSet = (VkDescriptorSet)bindlessData->descriptorSet->native;
        write.dstBinding = bindingIdx;
        write.dstArrayElement = handleIndex;
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageView = (VkImageView)view->native;
        imageInfo.imageLayout = (type == UniformType::StorageImage)
            ? VK_IMAGE_LAYOUT_GENERAL
            : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        write.pImageInfo = &imageInfo;
        vkUpdateDescriptorSets(ctx->device, 1, &write, 0, 0);

        return handleIndex;
    }

    uint32_t unbindBindlessImage(rs_context_vk* ctx, rs_bindless_data_vk* bindlessData, uint32_t index, bool uav)
    {
        //FIXME: this is only used for some /*VALIDATION CASE*/
        UniformType type = uav ? UniformType::StorageImage : UniformType::Texture;
        auto bindingTable = uav ? &bindlessData->storageImagesBinding : &bindlessData->texturesBinding;
        auto resource = bindingTable->get(index);
        if (!resource) {
            assert(false);
            return uav ?defalut_no_texture_UAV->defaultView->bindlessIndex : defalut_no_texture->defaultView->bindlessIndex;
        }
        auto resourceDefault = uav ? defalut_no_texture_UAV->defaultView : defalut_no_texture->defaultView ;
        uint32_t default_unbind_index = resourceDefault->bindlessIndex;
        auto refAfterDec = resource->bindingRef.fetch_sub(1);
		resourceDefault->bindingRef.fetch_add(1);
		if (refAfterDec > 1) {
            return default_unbind_index;
        }

        assert(refAfterDec >= 1);

        resource->bindlessIndex = INVALID_BINDLESS_INDEX;
        auto& pendingRemoveTable = uav ? bindlessData->pendingUnbindStorage : bindlessData->pendingUnbindTexture;
        pendingRemoveTable[ctx->LogicFrameFif].push_back(index);
		return default_unbind_index;
    }

    uint64_t unbindBindlessBuffer(rs_context_vk* ctx, rs_bindless_data_vk* bindlessData, uint64_t index, bool uav)
    {
        return defalut_no_buffer_UAV->gpuAddress;
    }

    uint32_t updateBindlessSampler(rs_context_vk* ctx, rs_bindless_data_vk* bindlessData, rs_sampler_vk* sampler)
    {
        if (!sampler) {
            return INVALID_BINDLESS_INDEX;
        }
        if (sampler->bindlessIndex != INVALID_BINDLESS_INDEX) {
            sampler->bindingRef.fetch_add(1);
            return sampler->bindlessIndex;
        }

		auto& bindingTable = bindlessData->samplersBinding;
        auto bindingIdx = toVkBindingPos(bindlessData->samplerBindlessPos).bindingIdx;
        auto ret = bindingTable.Allocate(ctx->curRenderFrame, sampler);
        if (ret == INVALID_BINDLESS_INDEX) {
            return INVALID_BINDLESS_INDEX;
        }
		sampler->bindingRef.fetch_add(1);

        VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.dstSet = (VkDescriptorSet)bindlessData->descriptorSet->native;
        write.descriptorCount = 1;
        write.dstBinding = bindingIdx;
        write.dstArrayElement = ret;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        VkDescriptorImageInfo imgInfo{};
        imgInfo.sampler = (VkSampler)sampler->native;
        write.pImageInfo = &imgInfo;
        vkUpdateDescriptorSets(ctx->device, 1, &write, 0, 0);
        return ret;
    }

    uint32_t unbindBindlessSampler(rs_context_vk* ctx, rs_bindless_data_vk* bindlessData, uint32_t index)
    {

        auto& bindingTable = bindlessData->samplersBinding;
        auto sampler = bindingTable.get(index);
		defalut_no_sampler->bindingRef.fetch_add(1);
		if (!sampler || sampler->bindlessIndex == INVALID_BINDLESS_INDEX) {
            assert(false);
            return defalut_no_sampler->bindlessIndex;
        }
        auto refAfterDec = sampler->bindingRef.fetch_sub(1);
        if (refAfterDec > 1) {
			return defalut_no_sampler->bindlessIndex;
        }
        assert(refAfterDec >= 1);
        bindlessData->pendingUnbindSampler[ctx->LogicFrameFif].push_back(index);
		return defalut_no_sampler->bindlessIndex;
    }

    uint64_t beginRsFrameVk(rs_context_vk* ctx, rs_bindless_data_vk* bindlessData)
    {
        ctx->nextRenderFrame++;
        uint64_t maxFif = ctx->maxFrameInFlight;
        auto cmdbufMgr = ctx->cmdBufferMgr;
        auto descriptorSetMgr = ctx->descriptorSetMgr;
        //Wait newRenderFrame % maxFif finish
        uint64_t toWaitFrame = ctx->nextRenderFrame % maxFif;
        ctx->LogicFrameFif = toWaitFrame;
        ctx->canRenderNextFrame = true;
        if (bindlessData) {
            beginFrameUnbindBindlessResource(ctx, bindlessData);
        }
        ctx->destroyer->endFrameDestroy(ctx, ctx->curRenderFrame);
        cmdbufMgr->beginFrame(ctx, ctx->nextRenderFrame);
        descriptorSetMgr->beginFrame(ctx, ctx->nextRenderFrame);
        if (ctx->nextRenderFrame == 0) {
            createDefaultResources(ctx);
        }
        return 0;
    }

    uint64_t beginRsRenderFrameVk(rs_context_vk* ctx)
    {
        ctx->curRenderFrame = ctx->nextRenderFrame;
        uint64_t maxFif = ctx->maxFrameInFlight;
        ctx->RenderFrameFif = ctx->curRenderFrame % maxFif;
        return 0;
    }

    uint64_t endRsFrameVk(rs_context_vk* ctx)
    {
        return ctx->nextRenderFrame;
    }

    uint32_t waitForNextPresentImage(rs_context_vk* ctx, rs_semaphore_vk* imageAvailableSignalSemaphore, rs_fence_vk* fenceToSignal)
    {
        uint32_t swapImageIdx = UINT32_MAX;
        VkSemaphore sem = imageAvailableSignalSemaphore == 0 ? VK_NULL_HANDLE : ((VkSemaphore*)imageAvailableSignalSemaphore->native)[getWaitFif(ctx, imageAvailableSignalSemaphore->waitFlag)];
        auto code = vkAcquireNextImageKHR(ctx->device, (VkSwapchainKHR)ctx->swapchain->native, UINT64_MAX, sem, VK_NULL_HANDLE, &swapImageIdx);
        if (code != VK_SUCCESS && code != VK_NOT_READY && code != VK_TIMEOUT) {
            assert(false && "Fail to get!");
        }
        return swapImageIdx;
    }

    void submitToPresentImage(rs_context_vk* ctx, uint32_t presentImgIdx, std::vector<rs_semaphore_vk*> canPresentToScreen)
    {
        //TODO: transit blitFrom -> transfer src
        //and signal a semaphore to present?
        std::vector<VkSemaphore> semphoresToWait;
        for (auto&& semRs : canPresentToScreen) {
            semphoresToWait.push_back( ((VkSemaphore*)semRs->native)[getWaitFif(ctx,semRs->waitFlag)]);
        }



        VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount;
        presentInfo.pWaitSemaphores = semphoresToWait.data();
        presentInfo.waitSemaphoreCount = semphoresToWait.size();
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = (VkSwapchainKHR*)&ctx->swapchain->native;
        presentInfo.pImageIndices = &presentImgIdx;
        auto presentRes = (vkQueuePresentKHR(ctx->presentQueue->queue, &presentInfo));
        if (presentRes != VK_SUCCESS) {
            if (presentRes == VK_ERROR_OUT_OF_DATE_KHR) {
                //NEED TO REBUILD SWAP CHAIN
            }
            else {
                assert(false);
                Log::error("Present error");
            }
        }
    }

    void WaitForDeviceIdel(rs_context_vk* ctx)
    {
        vkDeviceWaitIdle(ctx->device);
    }

    void cmdCopyBufferToImage(rs_commandbuffer_vk* cb, rs_context_vk* context, rs_image_vk* image, rs_buffer_vk* buffer ,uint32_t srcOffset, int x, int y, int z, int width, int height, int depth, uint32_t dstMip, int layeroff, int layerSize)
    {
        auto cmd = (VkCommandBuffer)cb->native;
        VkImageSubresourceLayers    imageSubresource{};
        //Most of time: color image data?
        imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageSubresource.mipLevel = dstMip;
        imageSubresource.baseArrayLayer = layeroff;
        imageSubresource.layerCount = layerSize;
        VkOffset3D                  imageOffset{ x,y,z };
        VkExtent3D                  imageExtent{ width,height,depth };
        VkBufferImageCopy2 imageCpy{ VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2 };
        imageCpy.bufferOffset = srcOffset;
        imageCpy.bufferRowLength = 0;
        imageCpy.bufferImageHeight = 0;
        imageCpy.imageSubresource = imageSubresource;
        imageCpy.imageOffset = imageOffset;
        imageCpy.imageExtent = imageExtent;
     
        VkCopyBufferToImageInfo2 cpInfo{ VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2 };
        cpInfo.srcBuffer = (VkBuffer)buffer->native;
        cpInfo.dstImage = (VkImage)image->native;
        cpInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        cpInfo.regionCount = 1;
        cpInfo.pRegions = &imageCpy;
        vkCmdCopyBufferToImage2(cmd, &cpInfo);
    }

    void cmdCopyImageToBuffer(rs_commandbuffer_vk* cb, rs_context_vk* context, rs_image_vk* image, rs_buffer_vk* buffer, uint32_t bufferOffset, int x, int y, int z, int width, int height, int depth, uint32_t mip, int layeroff, int layerSize)
    {
        auto cmd = (VkCommandBuffer)cb->native;
        VkImage vImg = (VkImage)image->native;

        VkImageAspectFlags copyAspectFlag = 0;

        if (image->usage & ImageUsage::ImageUsage_DepthStencilAttachment) {
            copyAspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        else {
            copyAspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
        }


        VkBufferImageCopy2 region{};
        region.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
        region.bufferOffset = bufferOffset;   
        region.bufferRowLength = 0;            
        region.bufferImageHeight = 0;        

        region.imageSubresource.aspectMask = copyAspectFlag;
        region.imageSubresource.mipLevel = mip;
        region.imageSubresource.baseArrayLayer = layeroff;
        region.imageSubresource.layerCount = layerSize;

        region.imageOffset = { (int32_t)x, (int32_t)y, (int32_t)z };
        region.imageExtent = { (uint32_t)width, (uint32_t)height, (uint32_t)depth };

        transitionImageState(cb, image, ResourceState::TransferSrc, mip, 1, layeroff, layerSize);
        transitionBufferState(cb, buffer, ResourceState::TransferDst);

        VkCopyImageToBufferInfo2 copyInfo{};
        copyInfo.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_TO_BUFFER_INFO_2;
        copyInfo.srcImage = vImg;
        copyInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        copyInfo.dstBuffer = (VkBuffer)buffer->native;
        copyInfo.regionCount = 1;
        copyInfo.pRegions = &region;

        vkCmdCopyImageToBuffer2(cmd, &copyInfo);

    }

}


