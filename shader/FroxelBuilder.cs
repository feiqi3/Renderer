#version 450
#extension GL_EXT_shader_image_load_formatted : require
#extension GL_EXT_samplerless_texture_functions : require
#extension GL_EXT_buffer_reference : require
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

#define VAR_ARR_SIZE 1
#include "ShaderResource.inl"
#include "FroxelHelper.inl"
#include "SceneLightsCullData.h"
//Follow the guide from filament doc
//8.4.13.10 From depth to froxel   

//z:  current linear z
//n:  near plane of camera
//f:  far plane of camera
//sn: special near plane value that the z under this value will be put into froxel bin 0 along z axis
//m:  max froxel to get along z axis      


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


DECL_BUFFER_STD430_BEG(Froxels)
    FroxelInfo         froxelList[VAR_ARR_SIZE];
DECL_BUFFER_STD430_END

RESOURCE_DECL_BEG(1)
SLOT_BUFFER_STD430(1,  Froxels,                  SSBO_froxelList)
SLOT_BUFFER_STD430(1,  FroxelConfigData,         SSBO_froxelConfig)
RESOURCE_DECL_END

#define FroxelList GetBuffer(SSBO_froxelList)
#define FroxelConfig GetBuffer(SSBO_froxelConfig)
vec3 unprojectToViewSpace(vec2 ndc) {
    vec4 clipPoint = vec4(ndc, 1.0, 1.0);
    vec4 viewPoint = FroxelConfig.invProjMat * clipPoint;
    return normalize(viewPoint.xyz / viewPoint.w);
}

void main(){
    ivec3 froxelIndex = ivec3(gl_GlobalInvocationID.xyz);   

    int tileX = froxelIndex.x;
    int tileY = froxelIndex.y;
    int tileZ = froxelIndex.z;

    if (tileX >= int(FroxelConfig.tileXMax) || 
        tileY >= int(FroxelConfig.tileYMax) || 
        tileZ >= int(FroxelConfig.tileZMax)) {
        return;
    }


    float tileXStepNDC = 1. / FroxelConfig.tileXMax;
    float tileYStepNDC = 1. / FroxelConfig.tileYMax;
    float xInNdc = (tileXStepNDC * tileX) * 2.f - 1.f;
    float yInNdc = (tileYStepNDC * tileY) * 2.f - 1.f;
    float x_1InNdc = (tileXStepNDC * (tileX + 1)) * 2.f - 1.f;
    float y_1InNdc = (tileYStepNDC * (tileY + 1)) * 2.f - 1.f;

    float spN = FroxelConfig.specialNear;
    float camNear = FroxelConfig.camNear;
    float camFar  = FroxelConfig.camZFar;
    int binCount = int(FroxelConfig.tileZMax);
    vec2 zView = -binToZLinear(tileZ,spN,camNear,camFar,float(binCount));

//All rays from 0,0 to view clip point
    vec3 rayBL = unprojectToViewSpace(vec2(xInNdc,   yInNdc));   
    vec3 rayBR = unprojectToViewSpace(vec2(x_1InNdc, yInNdc));   
    vec3 rayTL = unprojectToViewSpace(vec2(xInNdc,   y_1InNdc)); 
    vec3 rayTR = unprojectToViewSpace(vec2(x_1InNdc, y_1InNdc)); 

    vec3 normalLeft   = normalize(cross(rayBL, rayTL));
    vec3 normalRight  = normalize(cross(rayTR, rayBR));
    vec3 normalTop    = normalize(cross(rayTL, rayTR));
    vec3 normalBottom = normalize(cross(rayBR, rayBL));

    int binIndex = getBinIndex(froxelIndex, FroxelConfig.tileXMax, FroxelConfig.tileYMax, FroxelConfig.tileZMax);
    FroxelList.froxelList[binIndex].leftNormal   = vec4(normalLeft, 0.0);   
    FroxelList.froxelList[binIndex].rightNormal  = vec4(normalRight, 0.0);   
    FroxelList.froxelList[binIndex].topNormal    = vec4(normalTop, 0.0);   
    FroxelList.froxelList[binIndex].bottomNormal = vec4(normalBottom, 0.0);   
    // < 0
    FroxelList.froxelList[binIndex].zNear        = zView.x;
    FroxelList.froxelList[binIndex].zFar         = zView.y;
}
