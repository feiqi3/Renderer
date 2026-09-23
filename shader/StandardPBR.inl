#ifndef STANDARD_PBR_INL_
#define STANDARD_PBR_INL_

#ifdef SKIN
DECL_BUFFER_STD430_BEG(SkeletonAnimationInfoList)
int jointsNum;
int padding0;
int padding1;
int padding2;
mat4 JointMulIBM[VAR_ARR_SIZE];
DECL_BUFFER_STD430_END
#endif//SKIN


DECL_RBUFFER_STD430_BEG(PBRDataAlias)
    PBRData pbrData;
DECL_RBUFFER_STD430_END

RESOURCE_DECL_BEG(4)
SLOT_TEXTURE(4, texture2D, u_baseColorTex)
SLOT_SAMPLER(4, sampler, u_baseColorSampler)
SLOT_TEXTURE(4, texture2D, u_normalTex)
SLOT_SAMPLER(4, sampler, u_normalSampler)
SLOT_TEXTURE(4, texture2D, u_metallicRoughnessTex)
SLOT_SAMPLER(4, sampler, u_metallicRoughnessSampler)
SLOT_TEXTURE(4, texture2D, u_AOTex)
SLOT_SAMPLER(4, sampler, u_AOSampler)
SLOT_CONST_BUFFER(4, PBRDataAlias, CBUFFER_pbrData)
#ifdef SKIN
SLOT_BUFFER_STD430(4, SkeletonAnimationInfoList, u_anmInfoList)
#endif //SKIN
RESOURCE_DECL_END

#endif//STANDARD_PBR_INL_