#ifndef FROXEL_DATA_H_
#define FROXEL_DATA_H_
#include "GPUSharedDef.h"

GPU_SHARED_NAMESPACE_BEGIN

    GPU_STRUCT_BEGIN(FroxelInfo)
    //All planes shares one point 0,0,0
    vec4 leftNormal;   
    vec4 rightNormal;  
    vec4 topNormal;    
    vec4 bottomNormal; 
    float zNear;       
    float zFar;        
    float padding0;    
    float padding1;
    GPU_STRUCT_END


	GPU_STRUCT_BEGIN(FroxelConfigData)
    float camZFar;
    float camNear;
    float specialNear;
    int   tileXMax;
    int   tileYMax;
    int   tileZMax;
    float padding0;
    float padding1;
    mat4  ProjMat;
    mat4  invProjMat;
    mat4  viewMat;
    mat4  viewProjectionMat;
	GPU_STRUCT_END
GPU_SHARED_NAMESPACE_END

#endif //FROXEL_DATA_H_
