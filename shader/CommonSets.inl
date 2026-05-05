#include "CameraData.h"
#include "SceneData.h"
#include "ObjectData.h"
layout(set = 1, binding = 0) uniform blockCamera {
	GPUCameraData		camera;
}CameraCommon;
 layout(set = 2, binding = 0)  uniform blockScene {
 	GPUSceneLightData	sceneLights;
 }SceneCommon;

 layout(set = 2, binding = 1 ) uniform textureCube 		PreFilterEnvMap;
 layout(set = 2, binding = 2 ) uniform texture2D 		BRDFLut;
 layout(set = 2, binding = 3 ) uniform sampler			SceneTextureSampler; 

layout(set = 3, binding = 0) uniform UniformBufferObject {
    ObjectCommonData ObjData;
} ObjData;

#define OBJDATA         ObjData.ObjData
#define CAMDATA         CameraCommon.camera
#define LIGHTDATA       SceneCommon.sceneLights

#include "ShaderResource.inl"
#include "BindlessSet.inl"