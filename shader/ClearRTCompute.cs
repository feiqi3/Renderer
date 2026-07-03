#version 450
#extension GL_EXT_shader_image_load_formatted : require

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(set = 0,binding = 0) uniform UBOBlock{
    vec4 clearCol;
}clearData;
layout(set = 0,binding = 1) uniform writeonly image2D outClear;
void main() {
        ivec2 imgSizeOut = imageSize(outClear);
    
    if (gl_GlobalInvocationID.x >= imgSizeOut.x || 
        gl_GlobalInvocationID.y >= imgSizeOut.y) {
        return;
    }
    imageStore(outClear, ivec2(gl_GlobalInvocationID.xy), clearData.clearCol);
}