#include "CameraData.h"
#include "SceneData.h"
#include "ObjectData.h"
#include "ShadowData.h"

layout(set = 1, binding = 0) uniform blockCamera {
	GPUCameraData		camera;
}CameraCommon;

//set 2 Scene,light related
layout(set = 2, binding = 0)  uniform blockScene {
 	GPUSceneLightData	sceneLights;
 }SceneCommon;

 layout(set = 2, binding = 1 ) uniform sampler		SceneTextureSampler; 

 layout(set = 2, binding = 2 ) uniform textureCube 		PreFilterEnvMap;
 layout(set = 2, binding = 3 ) uniform texture2D 		BRDFLut;

//Object related, every thing that unique to an object an put into this set
layout(set = 3, binding = 0) uniform UniformBufferObject {
    ObjectCommonData ObjData;
} ObjData;

//Set 4: Material related
//.......
#if !defined(NO_SHADOW)
//Set 5: Shadow related //Perhaps light in data
layout(set = 5, binding = 0) uniform UniformBufferShadow {
    GPUSceneShadowData ShadowData;
} ShadowData;
layout(set = 5, binding = 1 ) uniform texture2D 		DirShadowMap;
layout(set = 5, binding = 2 ) uniform sampler			ShadowSampler; 
#endif//NO_SHADOW




#define OBJDATA         ObjData.ObjData
#define CAMDATA         CameraCommon.camera
#define LIGHTDATA       SceneCommon.sceneLights
#define SHADOWDATA      ShadowData.ShadowData
#include "ShaderResource.inl"
#include "BindlessSet.inl"