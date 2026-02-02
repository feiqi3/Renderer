#include "CameraData.h"
#include "SceneData.h"
layout(set = 0, binding = 0) uniform blockCamera {
	GPUCameraData		camera;
}CameraCommon;
layout(set = 1, binding = 0)  uniform blockScene {
	GPUSceneLightData	sceneLights;
}SceneCommon;