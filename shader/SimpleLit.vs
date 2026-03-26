#include "CommonSets.inl"

#include "PBREntity.h"

layout(set = 2, binding = 1) uniform UniformBufferObject {
    PBRData pbrData
} CBUFFER_pbrData;

