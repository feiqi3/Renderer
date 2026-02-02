#ifndef PBR_ENTITY_H_
#define PBR_ENTITY_H_

#include "GPUSharedDef.h"

GPU_SHARED_NAMESPACE_BEGIN
	GPU_STRUCT_BEGIN(PBRData)
        mat4 worldMatrix;
        mat4 invWorldMatrix;
	GPU_STRUCT_END
GPU_SHARED_NAMESPACE_END

#endif