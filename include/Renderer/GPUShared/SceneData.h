#ifndef SCENE_DATA_H_
#define SCENE_DATA_H_

#include "GPUSharedDef.h"
#define RENDER_MAX_LIGHT_PER_SCENE 8

GPU_SHARED_NAMESPACE_BEGIN
	GPU_STRUCT_BEGIN(GPULightData)
		// ----------------------------------------------------
		// x, y, z: Light position (Point/Spot) or 0 (for Directional)
		// w:		LightType (0=Dir, 1=Point, 2=Spot)
		vec4 positionType;

		// x, y, z: Light Direction (Directional/Spot)
		// w:       Radius
		vec4 directionRange;

		// x, y, z: Albedo (RGB)
		// w:       Intensity
		vec4 colorIntensity;

		// x: (Inner Cone cos)
		// y: (Outer Cone cos)
		// z, w: padding
		vec4 spotParams;
	GPU_STRUCT_END


	GPU_STRUCT_BEGIN(GPUSceneLightData)
		// x. Prefilter ENV MAP mips | y z w padding
		vec4 IBLControl;
		// x: total light num, y, z, w unused
		vec4 sceneLightInfo;
		GPULightData lights[RENDER_MAX_LIGHT_PER_SCENE];
	GPU_STRUCT_END

GPU_SHARED_NAMESPACE_END

#endif //SCENE_DATA_H_