#version 450   

//Image format can be omitted when this extension was enabled
#extension GL_EXT_shader_image_load_formatted : require

#include "CommonMath.inl"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(set = 0, binding = 0) uniform texture2D   blitFrom;
layout(set = 0, binding = 1) uniform writeonly  image2D blitTo;

layout(set = 0, binding = 2) uniform sampler blitSampler;

void main() {
    ivec2 imgSize = ivec2(imageSize(blitTo));
    if(gl_GlobalInvocationID.x >= imgSize.x || gl_GlobalInvocationID.y >= imgSize.y) {
        return;
    }
    vec2 curUV = (vec2(gl_GlobalInvocationID.xy) + vec2(0.5)) / vec2(imgSize);
    vec4 color = texture(sampler2D(blitFrom, blitSampler), curUV);
    imageStore(blitTo, ivec2(gl_GlobalInvocationID.xy), color);
}
