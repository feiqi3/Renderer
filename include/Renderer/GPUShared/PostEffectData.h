#ifndef POST_EFFECT_DATA_H_
#define POST_EFFECT_DATA_H_

#include "GPUSharedDef.h"

GPU_SHARED_NAMESPACE_BEGIN

    GPU_STRUCT_BEGIN(PostEffectConfig)
        float               BloomStrength;          //This parameter controlls the blend factor between Main rt and bloom rt
        float               Padding0;
        float               Padding1;
        float               Padding2;

        
    GPU_STRUCT_END

GPU_SHARED_NAMESPACE_END

#endif //POST_EFFECT_DATA_H_