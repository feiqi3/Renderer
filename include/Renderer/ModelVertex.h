#ifndef MODEL_VERTEX_H
#define MODEL_VERTEX_H
#include "common/CommonMath.h"
namespace Render {
	struct StandardModelVertex {
		vec3		position;
		vec3		normal;
		vec4		tangent;
		vec2		uv_0;
		uint32_t	color_u8x4_pack;
	};
}

#endif