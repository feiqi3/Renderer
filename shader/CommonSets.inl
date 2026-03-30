#include "CameraData.h"
#include "SceneData.h"
#include "ObjectData.h"
layout(set = 0, binding = 0) uniform blockCamera {
	GPUCameraData		camera;
}CameraCommon;
 layout(set = 1, binding = 0)  uniform blockScene {
 	GPUSceneLightData	sceneLights;
 }SceneCommon;
layout(set = 2, binding = 0) uniform UniformBufferObject {
    ObjectCommonData ObjData;
} ObjData;

#define PBRDATA         CBUFFER_pbrData.pbrData
#define OBJDATA         ObjData.ObjData
#define CAMDATA         CameraCommon.camera
#define LIGHTDATA       SceneCommon.sceneLights