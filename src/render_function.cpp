#include "render_function.h"
#include "vulkan/vulkan_render_function.h"
namespace Render{
	size_t getRsImageGPUSize(rs_image* image)
	{
		return Vulkan::getRsImageSize((Vulkan::rs_image_vk*)image);
	}
	bool queryImgFormatCaps(rs_context * ctx, ImageFormat fmt, FormatCapFlag flags)
{
	uint32_t uFmt = (uint32_t)fmt;
	if (uFmt >= uint32_t(ImageFormat::Invalid)) {
		return false;
	}
	return (ctx->ImageFormatCaps[uFmt] & flags) == flags;
}

RenderTextureFormat fromImageFormatToRtFormat(rs_context* ctx, ImageFormat fmt)
{
	for (int i = 0;i < (int)RenderTextureFormat::Invalid;++i) {
		if (ctx->rtFormatMap[i] == fmt) {
			return (RenderTextureFormat)i;
		}
	}
	return RenderTextureFormat::Invalid;
}

Render::ImageViewKey genViewKey(ImageType viewType, ViewAspect aspect, uint16_t baseMip, uint16_t mipCnt, uint16_t baseLayer, uint16_t layerCnt,UAVAccess uav)
{
	ImageViewKey key{};
	key.value = 0;
	key.bits.viewType = (uint16_t)viewType;
	key.bits.aspect = (uint16_t)aspect;
	key.bits.baseMip = baseMip;
	key.bits.mipCount = mipCnt;
	key.bits.baseLayer = baseLayer;
	key.bits.layerCount = layerCnt;
	key.bits.uavAccess = (uint16_t)uav;
	return key;
}

bool operator==(const ImageViewKey& keyA, const ImageViewKey& keyB)
{
	return keyA.value == keyB.value;
}

bool operator!=(const ImageViewKey& keyA, const ImageViewKey& keyB)
{
	return keyA.value != keyB.value;
}

ImageFormat fromRtFormatToImageFormat(rs_context * ctx,RenderTextureFormat fmt)
{
	const auto& map = ctx->rtFormatMap;
	auto uFmt = uint32_t(fmt);
	if (uFmt >= uint32_t(RenderTextureFormat::Invalid))return ImageFormat::Invalid;
	return map[uFmt];
}
bool isRtFormatHDRFormat(RenderTextureFormat fmt)
{
	return fmt == RenderTextureFormat::RGBA16F || fmt == RenderTextureFormat::RGBA32F;
}
uint32_t vertexFormatToSize(VertexFormat format)
{
	switch (format)
	{
	case VertexFormat::Float:   return 4;
	case VertexFormat::Float2:   return 8;
	case VertexFormat::Float3:   return 12;
	case VertexFormat::Float4:   return 16;

	case VertexFormat::Half2:    return 4;
	case VertexFormat::Half4:    return 8;

	case VertexFormat::Uint4:    return 16;
	case VertexFormat::UByte4N:  return 4;
	default: return 0;
	}
}
ImageFormat fromVertexFormatToImageFormat(VertexFormat fmt)
{
	switch (fmt)
	{
	case VertexFormat::Float:  return ImageFormat::R32_SFLOAT;
	case VertexFormat::Float2:  return ImageFormat::RG32_SFLOAT;
	case VertexFormat::Float3:  return ImageFormat::RGB32_SFLOAT;
	case VertexFormat::Float4:  return ImageFormat::RGBA32_SFLOAT;

	case VertexFormat::Half2:   return ImageFormat::RG16_SFLOAT;
	case VertexFormat::Half4:   return ImageFormat::RGBA16_SFLOAT;

	case VertexFormat::UByte4N: return ImageFormat::RGBA8_UNORM;
		// 明确禁止
	case VertexFormat::Uint4:
		return ImageFormat::Invalid;

	default:
		return ImageFormat::Invalid;
	}
}
VertexFormat fromImaegFormatToVertexFormat(ImageFormat fmt)
{
	switch (fmt)
	{
	case ImageFormat::R32_SFLOAT:      return VertexFormat::Float;
	case ImageFormat::RG32_SFLOAT:     return VertexFormat::Float2;
	case ImageFormat::RGB32_SFLOAT:    return VertexFormat::Float3;
	case ImageFormat::RGBA32_SFLOAT:   return VertexFormat::Float4;

	case ImageFormat::RG16_SFLOAT:     return VertexFormat::Half2;
	case ImageFormat::RGBA16_SFLOAT:  return VertexFormat::Half4;

	case ImageFormat::RGBA8_UNORM:     return VertexFormat::UByte4N;

	default:
		// depth / srgb / integer / compressed 都不应作为 vertex
		return VertexFormat::Invalid;
	}
}
}