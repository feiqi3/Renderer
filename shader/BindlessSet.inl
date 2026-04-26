#include "BindlessGlobalDefShared.h"
#ifdef BINDLESS_ENABLE

layout(set = BINDLESS_SET_IDX, binding = BUFFER_BINDLESS_ARRAY_BINDING_IDX) buffer GlobalUAVBuffersFormat { 
    uint data[]; 
} GlobalUAVBuffers[MAX_BUFFER_BINDLESS];

layout(set = BINDLESS_SET_IDX, binding = UAV_IMAGE_BINDLESS_ARRAY_BINDING_IDX) uniform image2D GlobalUAVImages[MAX_STORAGEIMAGE_BINDLESS];

layout(set = BINDLESS_SET_IDX, binding = SAMPLER_BINDLESS_ARRAY_BINDING_IDX) uniform sampler GlobalSamplers[MAX_SAMPLER_BINDLESS];

layout(set = BINDLESS_SET_IDX, binding = TEXTURE_BINDLESS_ARRAY_BINDING_IDX) uniform texture2D GlobalTextures[];

#endif 
