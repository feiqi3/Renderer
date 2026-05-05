#include "BindlessGlobalDefShared.h"
#ifdef BINDLESS_ENABLE
#extension GL_EXT_shader_image_load_formatted : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference : require
layout(set = BINDLESS_SET_IDX, binding = UAV_IMAGE_BINDLESS_ARRAY_BINDING_IDX) uniform image2D GlobalUAVImages[MAX_STORAGEIMAGE_BINDLESS];

layout(set = BINDLESS_SET_IDX, binding = SAMPLER_BINDLESS_ARRAY_BINDING_IDX) uniform sampler GlobalSamplers[MAX_SAMPLER_BINDLESS];

layout(set = BINDLESS_SET_IDX, binding = TEXTURE_BINDLESS_ARRAY_BINDING_IDX) uniform texture2D GlobalTextures[MAX_TEXTURE_BINDLESS];

#endif 
