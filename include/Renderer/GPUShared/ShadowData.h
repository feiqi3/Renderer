#ifndef SHADOW_DATA_H_
#define SHADOW_DATA_H_
#include "GPUSharedDef.h"
#include "SceneLightsDef.h"

GPU_SHARED_NAMESPACE_BEGIN
	GPU_STRUCT_BEGIN(GPUDirLightShadowCascadedData)
		mat4 ViewMat;
		mat4 ProjMat;
		vec4 OrthoSize;		//x,y ortho size(light camera projection plane size)
		vec4 Info;	//x: z depth end in main cam space
		GPU_STRUCT_END

	GPU_STRUCT_BEGIN(GPUDirLightShadowData)
		GPUDirLightShadowCascadedData cascadedShadowData[MAX_CASCADED_LAYERS];
		vec4 AtlasInfo;     //x,y -> Resolution, z cascaded layers, w interpolate factor
		vec4 LightDir;		//Direction of light. From surface point to light
	GPU_STRUCT_END

    GPU_STRUCT_BEGIN(GPUSceneShadowData)
		vec4 ShadowInfo;	//x: ShadowEnable, y: ShadowTechnique
		GPUDirLightShadowData DirLightShadowInfo;
	GPU_STRUCT_END
GPU_SHARED_NAMESPACE_END

#endif//SHADOW_DATA_H_