#ifndef MIPMAPGEN_H_
#define MIPMAPGEN_H_
#include "GPUSharedDef.h"
GPU_SHARED_NAMESPACE_BEGIN
	GPU_STRUCT_BEGIN(MipMapGenCfg)
        uint mipsCount;
        uint numWorkGroups;// use dispatch z as total slice count
        uint workGroupOffset;
        uint imageSizeX;
        uint imageSizeY;
        float invImageSizeX;
        float invImageSizeY;
        uint unused0;//padding
	GPU_STRUCT_END
GPU_SHARED_NAMESPACE_END

#endif
