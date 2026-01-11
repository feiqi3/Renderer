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

Render::RenderTextureFormat fromImageFormatToRtFormat(rs_context* ctx, ImageFormat fmt)
{
	for (int i = 0;i < (int)RenderTextureFormat::Invalid;++i) {
		if (ctx->rtFormatMap[i] == fmt) {
			return (RenderTextureFormat)i;
		}
	}
	return RenderTextureFormat::Invalid;
}

ImageFormat Render::fromRtFormatToImageFormat(rs_context * ctx,RenderTextureFormat fmt)
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