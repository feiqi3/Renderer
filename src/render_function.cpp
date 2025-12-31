#include "render_function.h"
namespace Render{
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
}