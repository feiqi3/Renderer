#ifndef SHADOW_DATA_H_
#define SHADOW_DATA_H_
#include "GPUSharedDef.h"
#include "SceneLightsDef.h"
    
GPU_SHARED_NAMESPACE_BEGIN
	GPU_STRUCT_BEGIN(GPUDirLightShadowData)
		mat4 ViewMat;
		mat4 ProjMat;
		vec4 AtlasInfo;     //x,y -> Resolution
	GPU_STRUCT_END

    GPU_STRUCT_BEGIN(GPUSceneShadowData)
		vec4 ShadowInfo;	//x: ShadowEnable, y: ShadowTechnique
		GPUDirLightShadowData DirLightShadowInfo;
	GPU_STRUCT_END
GPU_SHARED_NAMESPACE_END

#endif//SHADOW_DATA_H_