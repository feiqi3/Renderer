#version 450 
#extension GL_EXT_shader_image_load_formatted : require
#extension GL_EXT_samplerless_texture_functions : require
#include "ShaderResource.inl"
#include "BindlessSet.inl"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

RESOURCE_DECL_BEG(1)
    SLOT_TEXTURE(   1,          texture2D,      u_lastZ     )
    SLOT_RWTEXTURE( 1,          image2D,        u_writeZ    )
RESOURCE_DECL_END    


//Dispath size as the target write image
void main(){
    ivec2 writeImgSize = imageSize(GetRWTexture(u_writeZ));

    ivec2 curPixel          = ivec2( gl_GlobalInvocationID.xy );

    if(curPixel.x >= writeImgSize.x || curPixel.y >= writeImgSize.y){
        return;
    }
    ivec2 sampleImageSize   = writeImgSize * 2;
    //Center of sample target
    ivec2 samplePointCenter = curPixel * 2;

    float sampledDepth[4]   = { 
        texelFetch( GetTexture(u_lastZ), samplePointCenter ,0 )            .x,
        texelFetch( GetTexture(u_lastZ), samplePointCenter + ivec2(1,0),0 ).x,
        texelFetch( GetTexture(u_lastZ), samplePointCenter + ivec2(1,1),0 ).x,
        texelFetch( GetTexture(u_lastZ), samplePointCenter + ivec2(0,1),0 ).x
    };

    float finalWrite = -1.;

    for(int i = 0;i < 4; ++ i){
        finalWrite = max(finalWrite,sampledDepth[i]);
    }
    vec4 writeVec = vec4(finalWrite,0.,0.,0.);
    imageStore(GetRWTexture(u_writeZ), curPixel ,writeVec);
}


