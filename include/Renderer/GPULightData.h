#ifndef GPU_LIGHT_DATA_H
#define GPU_LIGHT_DATA_H
#include "common/CommonMath.h"

#define RENDER_MAX_LIGHT_PER_SCENE 20

namespace Render {

	struct GPULightData {
		// ----------------------------------------------------
		// x, y, z: Light position (Point/Spot) or 0 (for Directional)
		// w:		LightType (0=Dir, 1=Point, 2=Spot)
		vec4 positionType;

		// x, y, z: Light Direction (Directional/Spot)
		// w:       Fade Range
		vec4 directionRange;

		// x, y, z: Albedo (RGB)
		// w:       Intensity
		vec4 colorIntensity;

		// x: (Inner Cone cos)
		// y: (Outer Cone cos)
		// z, w: padding
		vec4 spotParams;
	};


	struct GPUSceneLightData {
		// x: total light num, y, z, w unused
		vec4 sceneLightInfo;
		GPULightData lights[RENDER_MAX_LIGHT_PER_SCENE];
	};
}

#endif