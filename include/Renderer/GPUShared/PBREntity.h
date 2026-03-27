#ifndef PBR_ENTITY_H_
#define PBR_ENTITY_H_

#include "GPUSharedDef.h"

GPU_SHARED_NAMESPACE_BEGIN
	GPU_STRUCT_BEGIN(PBRData)
		//r g b a
		vec4 baseCol;
		//x: metallic y: roughness z: AO strength w: normal scale
		vec4 metalRoughAO;
		//xyz: Emissive factor, w: Texcoord id 0 or 1
		vec4 emissiveFactor;
		//x: baseCol tex id, y:metallic-roughness tex id, z: normal tex id, w: occlussion texcord id 0 or 1
		vec4 texControl;
	GPU_STRUCT_END
GPU_SHARED_NAMESPACE_END

#endif