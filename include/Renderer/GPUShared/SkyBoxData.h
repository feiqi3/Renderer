#ifndef SKYBOX_DATA_H_
#define SKYBOX_DATA_H_

#include "GPUSharedDef.h"

GPU_SHARED_NAMESPACE_BEGIN
	GPU_STRUCT_BEGIN(SkyBoxData)

    //1. rotate info:
    vec4 rotateQuat;
    //x,y,z color; w exposure
    vec4 colorexposureTuning; 

    GPU_STRUCT_END

GPU_SHARED_NAMESPACE_END

#endif