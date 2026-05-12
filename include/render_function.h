#ifndef RENDER_RESOURCE_FUNCTION
#define RENDER_RESOURCE_FUNCTION
#include "render_resource.h"
#include "render_resource_createinfo.h"

namespace Render {

	namespace Window {
		class rs_window;
	};
	uint32_t vertexFormatToSize(VertexFormat format);
	ImageFormat  fromVertexFormatToImageFormat(VertexFormat fmt);
	VertexFormat fromImaegFormatToVertexFormat(ImageFormat fmt);
	rs_context* initRsBackEnd(const BackEndInitDesc& desc);

	rs_buffer* createRsBuffer(rs_context* context,BufferDesc& desc);
	rs_buffer* destroyRsBuffer(rs_context* context,BufferDesc& desc);
	bool isRsBufferMappable(rs_context* context, rs_buffer* buffer);

	rs_image* createRsImage(rs_context* context, ImageDesc& desc);
	rs_image* destroyRsImage(rs_context* context, ImageDesc& desc);
	size_t getRsImageGPUSize(rs_image* image);

	rs_shader_module* createShader(rs_context* context, ShaderDesc& desc);
	rs_shader_module* destroyShader(rs_context* context, ShaderDesc& desc);
	bool queryImgFormatCaps(rs_context* ctx, ImageFormat fmt, FormatCapFlag flags);
	ImageFormat fromRtFormatToImageFormat(rs_context* ctx,RenderTextureFormat fmt);
	RenderTextureFormat fromImageFormatToRtFormat(rs_context* ctx, ImageFormat fmt);
	bool isHDRRtFormat(RenderTextureFormat fmt);

	ImageViewKey genViewKey(ImageType viewType, ViewAspect aspect, uint16_t baseMip, uint16_t mipCnt, uint16_t baseLayer, uint16_t layerCnt, UAVAccess uav);
};


#endif