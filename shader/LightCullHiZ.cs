#version 450  
#extension GL_EXT_shader_image_load_formatted : require
#extension GL_EXT_samplerless_texture_functions : require
#extension GL_EXT_buffer_reference : require

#include "ShaderResource.inl"
#include "SceneLightsCullData.h"
#include "BindlessSet.inl"

#define VAR_ARR_SIZE 1
#define LIGHT_PER_FROXEL 36

//Follow the guide from filament doc
//8.4.13.10 From depth to froxel   

//z:  current linear z
//n:  near plane of camera
//f:  far plane of camera
//sn: special near plane value that the z under this value will be put into froxel bin 0 along z axis
//m:  max froxel to get along z axis      

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

struct LightDataInfo{
    int          lightCount; 
    int          padding0;  
    int          padding1;  
    int          padding2;  
};

layout(set = 1) uniform MaterialData {
    LightDataInfo lightInfo;
} LightInfo;

DECL_BUFFER_STD430_BEG(LightDataList)
    GPULightData lightList[VAR_ARR_SIZE];
DECL_BUFFER_STD430_END

DECL_BUFFER_STD430_BEG(FroxelConfigData)
    mat4 viewMat;
    mat4 ProjMat;
    mat4 invProjMat;
    mat4 invViewProjMat;
    vec3 camPosition;
    float screenSizeX;
    float screenSizeY;
    float specialNear;
    float camNear;
    float camZFar;
    int tileXMax;
    int tileYMax;
    int tileZMax;
DECL_BUFFER_STD430_END

DECL_BUFFER_STD430_BEG(LightCulledDataList)
    int           lightCount; 
    float         padding0;
    float         padding1;
    int           padding2;  
    LightCullData lightList[VAR_ARR_SIZE];
DECL_BUFFER_STD430_END

RESOURCE_DECL_BEG(1)
//HiZ Texture to cull lights
    SLOT_TEXTURE        (1,    texture2D,               HizTex)
//HiZ Sampler to cull lights
    SLOT_SAMPLER        (1,    sampler,                 PointSampler)
    SLOT_BUFFER_STD430  (1,    LightDataList,           SSBO_lightList)
    SLOT_BUFFER_STD430  (1,    LightCulledDataList,     SSBO_passHiZlightDataList)
    SLOT_BUFFER_STD430  (1,    FroxelConfigData,        SSBO_froxelConfig)
RESOURCE_DECL_END

LightCullData GetLightCullData(int lightIndex, mat4 viewMat){
    GPULightData lightData = GetBuffer(SSBO_lightList).lightList[lightIndex];
    LightCullData cullData;
    cullData.aabbInViewMin = vec4(0.0);
    cullData.aabbInViewMax = vec4(0.0, 0.0, 0.0, float(lightIndex));

    //Dir light
    if(lightData.positionType.w == 0.0){
        return cullData;
    }
    vec3 centerInView = (viewMat * vec4(lightData.positionType.xyz, 1.0)).xyz;
    float radius = lightData.directionRange.w;
    
    cullData.aabbInViewMin = vec4(centerInView - vec3(radius), 0.0);
    cullData.aabbInViewMax = vec4(centerInView + vec3(radius), float(lightIndex));

    return cullData;
}


void WriteLightCullData(LightCullData cullData){
    int writeIndex = atomicAdd(GetBuffer(SSBO_passHiZlightDataList).lightCount, 1);
    GetBuffer(SSBO_passHiZlightDataList).lightList[writeIndex] = cullData;
}

//Cull light by hiz
//Inspired by doom graphics study

void main(){

    vec2 screenSize = vec2(GetBuffer(SSBO_froxelConfig).screenSizeX, GetBuffer(SSBO_froxelConfig).screenSizeY);

    int lightCount = LightInfo.lightInfo.lightCount;
    int curLightIdx = int(gl_GlobalInvocationID.x);
    if(lightCount == 0 || curLightIdx >= lightCount){
        return;
    }
    mat4 projMat = GetBuffer(SSBO_froxelConfig).ProjMat;

    GPULightData light = GetBuffer(SSBO_lightList).lightList[curLightIdx];

    LightCullData cullData = GetLightCullData(curLightIdx, GetBuffer(SSBO_froxelConfig).viewMat);
    
    int lightType = int(light.positionType.w);
    if (lightType == 0) {
        cullData.aabbInViewMax.w = -cullData.aabbInViewMax.w ;
        WriteLightCullData(cullData);
        return;
    }

    float nearZ = -GetBuffer(SSBO_froxelConfig).camNear;
    float zFarPoint  = cullData.aabbInViewMin.z; 
    float zNearPoint = cullData.aabbInViewMax.z; 
    if (zFarPoint >= nearZ) {
        return; 
    }
    if (zNearPoint >= nearZ) {
        WriteLightCullData(cullData);
        return;
    }

    vec3 points[8];
    GetAABBCorners(cullData, points);

    vec4 pointsInClip[8];
    vec2 minp = vec2(1000000,10000000);
    vec2 maxp = vec2(-1000000,-10000000);
    float minZ = 1000000;
    bool isForceNotCulled = false;

    for(int j = 0;j < 8; ++j){
        pointsInClip[j] = projMat * vec4(points[j], 1.0);

        pointsInClip[j] /= pointsInClip[j] .w;
        
        vec2 clipxy = pointsInClip[j].xy;
        
        clipxy.xy = clipxy * 0.5 + 0.5;
        pointsInClip[j].xy = clipxy.xy;
        minp.x = min(minp.x, pointsInClip[j].x);
        minp.y = min(minp.y, pointsInClip[j].y);
        maxp.x = max(maxp.x, pointsInClip[j].x);
        maxp.y = max(maxp.y, pointsInClip[j].y);
        minZ = min(minZ, pointsInClip[j].z);
    }
    minp = clamp(minp,vec2(0,0),vec2(1,1));
    maxp = clamp(maxp,vec2(0,0),vec2(1,1));
    float xSize = maxp.x - minp.x;
    float ySize = maxp.y - minp.y;
    vec2 sizeInScreen = vec2(xSize * screenSize.x, ySize * screenSize.y);
    float maxSizeScreen = max(sizeInScreen.x, sizeInScreen.y);

    //cause we want to sample a 2x2 blocks of pixel
    //and the 2x2 block of texel must cover the whole bounding box
    //So make D = sizeof(boundingBoxInScreenSpace)
    //And l = Lod level
    //So to cover the whole bounding box, we need to make 2 * 2^(l) >= D
    // ===> so l >= log_2^(D) - 1
    // ===> l = ceil(log_2^(D) - 1)     
    //-1 cause we start at mip1
    int hizLevel = int(ceil(log2(maxSizeScreen / 2. ))) - 1;
    hizLevel = max(hizLevel, 0);
    vec2 pixSize = vec2(1.0) / (screenSize / float(1 <<(hizLevel + 1)) );
    //CLIP BY HIZ
    vec2 uv[4] = {
        vec2(minp.x, minp.y),
        vec2(maxp.x, minp.y),
        vec2(maxp.x, maxp.y),
        vec2(minp.x, maxp.y)
    };
    bool isOccluded = false;
    for(int k = 0;k < 4 && !isForceNotCulled; ++k){
        float hizDepth = textureLod(sampler2D(GetTexture(HizTex),GetSampler(PointSampler)), uv[k], float(hizLevel)).x;
        //MinZ is the nearest point of the light aabb
        if(hizDepth < minZ){
            isOccluded = true;
            break;
        }
    }

    if(!isOccluded){
        WriteLightCullData(cullData);
    }
}