#ifndef RENDER_RESOURCE_FUNCTION
#define RENDER_RESOURCE_FUNCTION
#include "render_resource.h"
#include "render_resource_createinfo.h"

namespace Render {

	namespace Window {
		class rs_window;
	};

	rs_context* initRsBackEnd(const BackEndInitDesc& desc);

	rs_buffer* createRsBuffer(rs_context* context,BufferDesc& desc);
	rs_buffer* destroyRsBuffer(rs_context* context,BufferDesc& desc);
	bool isRsBufferMappable(rs_context* context, rs_buffer* buffer);

	rs_image* createRsImage(rs_context* context, ImageDesc& desc);
	rs_image* destroyRsImage(rs_context* context, ImageDesc& desc);

	rs_shader_module* createShader(rs_context* context, ShaderDesc& desc);
	rs_shader_module* destroyShader(rs_context* context, ShaderDesc& desc);
};

#endif