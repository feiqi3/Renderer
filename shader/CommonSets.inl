#include "CameraData.h"
#include "SceneData.h"
#include "ObjectData.h"
#include "ShadowData.h"
#include "SceneLightsCullData.h"
#if defined(SHADOW_PASS) || defined(PREZ)
#define NO_LIGHTS
#endif
layout(set = 1, binding = 0) uniform blockCamera {
	GPUCameraData		camera;
}CameraCommon;

//set 2 Scene,light related
layout(set = 2, binding = 0)  uniform blockScene {
 	GPUSceneLightData	sceneLights;
 }SceneCommon;

struct SceneLights {
    GPULightData list[1];
};

#if !defined(NO_LIGHTS)
 layout(set = 2, binding = 1 ) uniform sampler		SceneTextureSampler; 

 layout(set = 2, binding = 2 ) uniform textureCube 		PreFilterEnvMap;
 layout(set = 2, binding = 3 ) uniform texture2D 		BRDFLut;

 layout(set = 2, binding = 4 ,std430 ) buffer SceneLightsBuffer{
    SceneLights lightList;
}SceneLightsList; 

struct _ClusterLight {
    FroxelLightDataList lightLists[1];
};

 layout(set = 2, binding = 5 ,std430 ) buffer Block_FroxelLightDataList {
    _ClusterLight lights;
 }ClusteredLights;

 layout(set = 2, binding = 6 ) uniform ClusterInfoUBO {
    ClusterInfo Cluster;
} ClusterInfoData;

#endif //NO_LIGHTS

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
layout(set = 5, binding = 1 ) uniform texture2DArray    DirShadowMap;
layout(set = 5, binding = 2 ) uniform sampler			ShadowSampler; 
#endif//NO_SHADOW




#define OBJDATA         ObjData.ObjData
#define CAMDATA         CameraCommon.camera
#define LIGHTDATA       SceneCommon.sceneLights
#define SHADOWDATA      ShadowData.ShadowData
#define CLUSTERDATA     ClusterInfoData.Cluster
#include "ShaderResource.inl"
#include "BindlessSet.inl"