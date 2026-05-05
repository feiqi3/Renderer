#ifndef RENDER_COMMAND_H_
#define RENDER_COMMAND_H_
#include "render_resource_createinfo.h"
#include "common/CoreDefs.h"
namespace Render {
	struct rs_pipeline;
	struct rs_drawdata;
	class RenderEntity;
	struct RenderCommand {
		RenderEntity* entity;
		u64 renderMask;
		vec3 worldPos;
		u32 renderPriority;
	};
}

#endif