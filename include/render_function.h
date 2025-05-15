#ifndef RENDER_RESOURCE_FUNCTION
#define RENDER_RESOURCE_FUNCTION
#include "render_resource.h"
#include "render_resource_createinfo.h"
namespace Render {
	rs_buffer* createRsBuffer(rs_context* context,BufferDesc& desc);

	bool isRsBufferMappable(rs_context* context, rs_buffer* buffer);

	rs_image* createRsImage(rs_context* context, ImageDesc& desc);

	rs_shader_module* createShader(rs_context* context, ShaderDesc& desc);
};

#endif