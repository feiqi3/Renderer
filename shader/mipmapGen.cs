#version 450
#extension GL_GOOGLE_include_directive : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_group_vote : enable

##TODO: can use half version of spd instead of packed version. Which may gains a better performance

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

#define REDUCE_TYPE REDUCE_TYPE_AVG
#define REDUCE_TYPE_MAX 0
#define REDUCE_TYPE_MIN 1
#define REDUCE_TYPE_AVG 2

#define MAX_MIPS_SPD_GEN 12
#define MAX_SLICE_SPD_PROCESS 6

#define TARGET_IMAGE_TYPE_2D 0
#define TARGET_IMAGE_TYPE_2D_ARRAY 1

#if USE_TARGET_IMAGE_TYPE == TARGET_IMAGE_TYPE_2D
#define TARGET_IMAGE_TYPE image2D
#elif USE_TARGET_IMAGE_TYPE == TARGET_IMAGE_TYPE_2D_ARRAY
#define TARGET_IMAGE_TYPE image2DArray
#endif

// 0 -> scr image
// 1 to MAX_MIPS_SPD_GEN -> mip to store
layout(set=0, binding=0) uniform TARGET_IMAGE_TYPE imgDst[MAX_MIPS_SPD_GEN + 1];

#ifdef USE_SAMPLER
#define SPD_LINEAR_SAMPLER
layout(set=0, binding=1) uniform Sampler samplerSrc;
#endif

struct MipMapGenCfg{
    uint mipsCount;
    uint numWorkGroups;// use dispatch z as total slice count
    uint workGroupOffset;
    uint imageSizeX;
    uint imageSizeY;
    float invImageSizeX;
    float invImageSizeY;
};

struct _SPDGlobalAtomic{
    uint counter[MAX_SLICE_SPD_PROCESS];
};

## Mip Map generate control parameters
layout(set = 0, binding = 2) uniform UBOBlock {
    MipMapGenCfg cfg;
}MipMapGen;

layout(set = 0, binding = 3) coherent buffer spdGlobalAtomicBuffer {
    uint counter[MAX_SLICE_SPD_PROCESS];
}SpdGlobalAtomic;

#define MIP_GEN_CFG MipMapGen.cfg

#define A_GLSL
#define A_GPU
#include "3rd/ffx_a.h"
shared AU1 spdCounter;

shared AF1 spdIntermediateR[16][16];
shared AF1 spdIntermediateG[16][16];
shared AF1 spdIntermediateB[16][16];
shared AF1 spdIntermediateA[16][16];



AF4 SpdLoadSourceImage(ASU2 p, AU1 slice)
{
    vec2 imgSize = vec2(MIP_GEN_CFG.imageSizeX, MIP_GEN_CFG.imageSizeY);
    #if USE_TARGET_IMAGE_TYPE == TARGET_IMAGE_TYPE_2D_ARRAY
    p = clamp(p, AU2(0), AU2(imgSize) - AU2(1));
    return imageLoad(imgDst[0], ivec3(p,slice));
    #elif USE_TARGET_IMAGE_TYPE == TARGET_IMAGE_TYPE_2D
    p = clamp(p, AU2(0), AU2(imgSize) - AU2(1));
    return imageLoad(imgDst[0],p);
    #elif defined(SPD_LINEAR_SAMPLER)
    vec2 invSrcSize = vec2(MIP_GEN_CFG.invImageSizeX, MIP_GEN_CFG.invImageSizeY);
    vec2 uv = vec2(p) * invSrcSize + invSrcSize;
    return texture(Sampler2D(imgDst[0], samplerSrc), uv);
    #endif
}
AF4 SpdLoad(ASU2 p, AU1 slice)
{
    vec2 imgSize = imageSize(imgDst[5]);
    p = clamp(p, AU2(0), AU2(imgSize) - AU2(1));
    #if USE_TARGET_IMAGE_TYPE == TARGET_IMAGE_TYPE_2D_ARRAY
    return imageLoad(imgDst[5],ivec3(p,slice));
    #else
    return imageLoad(imgDst[5],p);
    #endif

}
void SpdStore(ASU2 p, AF4 value, AU1 mip, AU1 slice)
{
    #if USE_TARGET_IMAGE_TYPE == TARGET_IMAGE_TYPE_2D_ARRAY
    imageStore(imgDst[mip+1], ivec3(p,slice), value);
    #else
    imageStore(imgDst[mip+1], p, value);
    #endif
}
//For LDS atomic Counter
void SpdIncreaseAtomicCounter(AU1 slice)
{
    spdCounter = atomicAdd(SpdGlobalAtomic.counter[slice], 1);
}
AU1 SpdGetAtomicCounter()
{
    return spdCounter;
}

//For Global Atomic counter
void SpdResetAtomicCounter(AU1 slice)
{
    SpdGlobalAtomic.counter[slice] = 0;
}

AF4 SpdLoadIntermediate(AU1 x, AU1 y)
{
    return AF4(
    spdIntermediateR[x][y], 
    spdIntermediateG[x][y], 
    spdIntermediateB[x][y], 
    spdIntermediateA[x][y]);
}
void SpdStoreIntermediate(AU1 x, AU1 y, AF4 value)
{
    spdIntermediateR[x][y] = value.x;
    spdIntermediateG[x][y] = value.y;
    spdIntermediateB[x][y] = value.z;
    spdIntermediateA[x][y] = value.w;
}


float vectorElemMax(AF4 v0,AF4 v1,AF4 v2,AF4 v3,int elem)
{
    return  max(v0[elem],
            max(v1[elem], 
            max(v2[elem],
            v3[elem])));
}

float vectorElemMin(AF4 v0,AF4 v1,AF4 v2,AF4 v3,int elem)
{
    return  min(v0[elem],
            min(v1[elem], 
            min(v2[elem],
            v3[elem])));
}

//Reduce function: 
AF4 SpdReduce4(AF4 v0, AF4 v1, AF4 v2, AF4 v3)
{
    #if REDUCE_TYPE == REDUCE_TYPE_MAX
        AF4 ret = AF4(
            vectorElemMax(v0,v1,v2,v3,0),
            vectorElemMax(v0,v1,v2,v3,1),
            vectorElemMax(v0,v1,v2,v3,2),
            vectorElemMax(v0,v1,v2,v3,3)
        );
        return ret;
    #elif REDUCE_TYPE == REDUCE_TYPE_MIN
        AF4 ret = AF4(
            vectorElemMin(v0,v1,v2,v3,0),
            vectorElemMin(v0,v1,v2,v3,1),
            vectorElemMin(v0,v1,v2,v3,2),
            vectorElemMin(v0,v1,v2,v3,3)
        );
        return ret;
    #elif REDUCE_TYPE == REDUCE_TYPE_AVG
        return (v0+v1+v2+v3) * 0.25f;
    #else 
        return (v0+v1+v2+v3) * 0.25f;
    #endif
}

#include "3rd/ffx_spd.h"

void main()
{
       SpdDownsample(
        AU2(gl_WorkGroupID.xy), 
        AU1(gl_LocalInvocationIndex), 
        AU1(MIP_GEN_CFG.mipsCount), 
        AU1(MIP_GEN_CFG.numWorkGroups),
        AU1(gl_WorkGroupID.z),
        AU2(MIP_GEN_CFG.workGroupOffset)); 
}