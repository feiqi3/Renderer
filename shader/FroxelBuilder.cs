#version 450
#define VAR_ARR_SIZE 1
#include "FroxelHelper.inl"
#include "FroxelData.h"

//Follow the guide from filament doc
//8.4.13.10 From depth to froxel   

//z:  current linear z
//n:  near plane of camera
//f:  far plane of camera
//sn: special near plane value that the z under this value will be put into froxel bin 0 along z axis
//m:  max froxel to get along z axis      




layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

DECL_BUFFER_STD430_BEG(Froxels)
    FroxelInfo         froxelList[VAR_ARR_SIZE];
DECL_BUFFER_STD430_END

layout(set = 1,binding = 0) uniform UBOBlock{
    FroxelConfigData data;
}FroxelConfig;

RESOURCE_DECL_BEG(1)
SLOT_BUFFER_STD430(1,  Froxels,                  u_froxelData)
RESOURCE_DECL_END

vec3 unprojectToViewSpace(vec2 ndc) {
    vec4 clipPoint = vec4(ndc, 1.0, 1.0);
    vec4 viewPoint = FroxelConfig.data.invProjMat * clipPoint;
    return normalize(viewPoint.xyz / viewPoint.w);
}

void main(){
    ivec3 froxelIndex = gl_GlobalInvocationID.xyz;   

    if (tileX >= int(FroxelConfig.data.tileXMax) || 
        tileY >= int(FroxelConfig.data.tileYMax) || 
        tileZ >= int(FroxelConfig.data.tileZMax)) {
        return;
    }

    int tileX = froxelIndex.x;
    int tileY = froxelIndex.y;
    int tileZ = froxelIndex.z;
    float tileXStepNDC = 1. / FroxelConfig.data.tileXMax;
    float tileYStepNDC = 1. / FroxelConfig.data.tileYMax;
    float xInNdc = (tileXStepNDC * tileX) * 2.f - 1.f;
    float yInNdc = (tileYStepNDC * tileY) * 2.f - 1.f;
    float x_1InNdc = (tileXStepNDC * (tileX + 1)) * 2.f - 1.f;
    float y_1InNdc = (tileYStepNDC * (tileY + 1)) * 2.f - 1.f;

    float spN = FroxelConfig.data.specialNear;
    float camNear = FroxelConfig.data.camNear;
    float camFar  = FroxelConfig.data.camZFar;
    int binCount = FroxelConfig.data.tileZMax;
    vec2 zView = -binToZLinear(tileZ,spN,camNear,camFar,(float)binCount);

//All rays from 0,0 to view clip point
    vec3 rayBL = unprojectToViewSpace(vec2(xInNdc,   yInNdc));   
    vec3 rayBR = unprojectToViewSpace(vec2(x_1InNdc, yInNdc));   
    vec3 rayTL = unprojectToViewSpace(vec2(xInNdc,   y_1InNdc)); 
    vec3 rayTR = unprojectToViewSpace(vec2(x_1InNdc, y_1InNdc)); 

    vec3 normalLeft   = normalize(cross(rayBL, rayTL));
    vec3 normalRight  = normalize(cross(rayTR, rayBR));
    vec3 normalTop    = normalize(cross(rayTL, rayTR));
    vec3 normalBottom = normalize(cross(rayBR, rayBL));

    int binIndex = getBinIndex(froxelIndex, FroxelConfig.data.tileXMax, FroxelConfig.data.tileYMax, FroxelConfig.data.tileZMax);
    u_froxelData.froxelList[binIndex].leftNormal   = vec4(normalLeft, 0.0);   
    u_froxelData.froxelList[binIndex].rightNormal  = vec4(normalRight, 0.0);   
    u_froxelData.froxelList[binIndex].topNormal    = vec4(normalTop, 0.0);   
    u_froxelData.froxelList[binIndex].bottomNormal = vec4(normalBottom, 0.0);   
    // < 0
    u_froxelData.froxelList[binIndex].zNear        = zView.x;
    u_froxelData.froxelList[binIndex].zFar         = zView.y;
}
