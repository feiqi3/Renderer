#include "render_function.h"

bool Render::queryImgFormatCaps(rs_context* ctx, ImageFormat fmt, FormatCapFlag flags)
{
	uint32_t uFmt = (uint32_t)fmt;
	if (uFmt >= uint32_t(ImageFormat::Invalid)) {
		return false;
	}
	return (ctx->ImageFormatCaps[uFmt] & flags) == flags;
}
