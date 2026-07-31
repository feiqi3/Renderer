#version 450  
#extension GL_EXT_shader_image_load_formatted : require
#include "CommonMath.inl"
#include "SceneData.h"
#include "FroxelData.h"
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


DECL_BUFFER_STD430_BEG(LightDataList)
    GPULightData lightList[VAR_ARR_SIZE]
DECL_BUFFER_STD430_END

DECL_BUFFER_STD430_BEG(Froxels)
    FroxelInfo         froxelList[VAR_ARR_SIZE];
DECL_BUFFER_STD430_END

DECL_BUFFER_STD430_BEG(FroxelLightData)
    uint         lightCount;
    uint         lightIndex[LIGHT_PER_FROXEL];
DECL_BUFFER_STD430_END

RESOURCE_DECL_BEG(1)
    SLOT_CONST_BUFFER(1,    texture2D,       HizTex)
    SLOT_CONST_BUFFER(1,    LightDataList,   SSBO_lightDataList)
    SLOT_CONST_BUFFER(1,    FroxelLightData, SSBO_froxelLightData)
    SLOT_CONST_BUFFER(1,    Froxels,         SSBO_froxelList)
RESOURCE_DECL_END

//Cull light by hiz
//Inspired by doom graphics study

void main(){
    //TODO:
    
    //1. find lights affect this froxel
    //1.1 Simple cull lights in this froxel by tilexXY
    //1.2 CullLight by plane/sphere test(and we already have normal in froxel, use (0,0,0) | camera origin as the plane's point; for froxel near plane and far plane, its normal is always along side the z-axis)

    //2. find current froxel hiz level, and cull lights by hiZ

    //2. write lights into froxel

}