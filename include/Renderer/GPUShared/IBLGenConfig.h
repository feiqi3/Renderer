#ifndef IBLGEN_CONFIG_H_
#define IBLGEN_CONFIG_H_
#include "GPUSharedDef.h"
GPU_SHARED_NAMESPACE_BEGIN
	GPU_STRUCT_BEGIN(PrefilterEnvMapCfg)
    float curRoughness;
    float IsHDR;
    uint padding1;
    uint padding2;
	GPU_STRUCT_END
GPU_SHARED_NAMESPACE_END

#endif