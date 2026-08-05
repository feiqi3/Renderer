#version 450  
#extension GL_EXT_shader_image_load_formatted : require
#include "CommonMath.inl"
#include "ShaderResource.inl"
#include "BindlessSet.inl"
#include "SceneData.h"
#include "FroxelHelper.inl"
#include "SceneLightsCullData.h"

#define VAR_ARR_SIZE 1
#define LIGHT_PER_FROXEL 36

//Follow the guide from filament doc
//8.4.13.10 From depth to froxel   

//z:  current linear z
//n:  near plane of camera
//f:  far plane of camera
//sn: special near plane value that the z under this value will be put into froxel bin 0 along z axis
//m:  max froxel to get along z axis      

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

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

DECL_BUFFER_STD430_BEG(Froxels)
    FroxelInfo   froxelList[VAR_ARR_SIZE];
DECL_BUFFER_STD430_END

DECL_BUFFER_STD430_BEG(FroxelLightData)
    FroxelLightDataList froxelLightList[VAR_ARR_SIZE];
DECL_BUFFER_STD430_END

RESOURCE_DECL_BEG(1)
// Pass 1 After hiz culled
    SLOT_BUFFER_STD430  (1,    LightCulledDataList, SSBO_passHiZlightDataList)
// Froxel output
    SLOT_BUFFER_STD430   (1,    FroxelLightData,     SSBO_froxelLightData)
    SLOT_BUFFER_STD430   (1,    Froxels,             SSBO_froxelList)
    SLOT_BUFFER_STD430   (1,    FroxelConfigData,    SSBO_froxelConfig)
RESOURCE_DECL_END

bool IsAABBOutsidePlane(vec3 N, AABB aabb) {
    //Min point in the direction of the plane normal
    vec3 nVertex = vec3(
        (N.x < 0.0) ? aabb.maxP.x : aabb.minP.x,
        (N.y < 0.0) ? aabb.maxP.y : aabb.minP.y,
        (N.z < 0.0) ? aabb.maxP.z : aabb.minP.z
    );

    //Out sizeof the plane?(The plane is go through the origin)
    return dot(N, nVertex/* - vec3(0,0,0)*/) > 0.0;
}

bool FroxelTestFast(in LightCullData cullData, in FroxelInfo froxelInfo) {
    //Cause z is neg.
    float distFar  = -(cullData.aabbInViewMin.z); 
    float distNear = -(cullData.aabbInViewMax.z); 
    if (distNear > froxelInfo.zFar || 
        distFar < froxelInfo.zNear) {
        return false;
    }

    AABB aabb;
    aabb.minP = cullData.aabbInViewMin.xyz;
    aabb.maxP = cullData.aabbInViewMax.xyz;

    //Just test 4 plane
    if (IsAABBOutsidePlane(froxelInfo.leftNormal.xyz,   aabb)) return false;
    if (IsAABBOutsidePlane(froxelInfo.rightNormal.xyz,  aabb)) return false;
    if (IsAABBOutsidePlane(froxelInfo.topNormal.xyz,    aabb)) return false;
    if (IsAABBOutsidePlane(froxelInfo.bottomNormal.xyz, aabb)) return false;

    return true;
}

void main() {
    int froxelTileXMax = GetBuffer(SSBO_froxelConfig).tileXMax;
    int froxelTileYMax = GetBuffer(SSBO_froxelConfig).tileYMax;
    int froxelTileZMax = GetBuffer(SSBO_froxelConfig).tileZMax;

    ivec3 curFroxelTile = ivec3(gl_GlobalInvocationID.xyz);
    if (curFroxelTile.x >= froxelTileXMax || curFroxelTile.y >= froxelTileYMax || curFroxelTile.z >= froxelTileZMax) {
        return;
    }

    int binIndex = getBinIndex(curFroxelTile, froxelTileXMax, froxelTileYMax, froxelTileZMax);
    GetBuffer(SSBO_froxelLightData).froxelLightList[binIndex].lightCount = 0;

    FroxelInfo curFroxelInfo = GetBuffer(SSBO_froxelList).froxelList[binIndex];

    // total light culled by hiz
    int culledLightCount = GetBuffer(SSBO_passHiZlightDataList).lightCount;

    // place lights in hiz pass into froxel
    for (int i = 0; i < culledLightCount; ++i) {
        LightCullData cullData = GetBuffer(SSBO_passHiZlightDataList).lightList[i];

        if (FroxelTestFast(cullData, curFroxelInfo)) {
            uint curLightCount = GetBuffer(SSBO_froxelLightData).froxelLightList[binIndex].lightCount;
            if (curLightCount < LIGHT_PER_FROXEL) {
                //Get Original light index
                int originalLightIndex = int(cullData.aabbInViewMax.w);
                
                GetBuffer(SSBO_froxelLightData).froxelLightList[binIndex].lightIndex[curLightCount] = originalLightIndex;
                GetBuffer(SSBO_froxelLightData).froxelLightList[binIndex].lightCount++;
            }
        }
    }
}