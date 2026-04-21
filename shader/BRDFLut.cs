#version 450   

//Image format can be omitted when this extension was enabled
#extension GL_EXT_shader_image_load_formatted : require

#include "CommonMath.inl"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(set = 0,binding = 0) uniform writeonly image2D OutBrdfLUT;

vec4 ImportanceSampleGGX( vec2 E, float Roughness4 )
{
    float m2 = Roughness4;

    float Phi = 2 * PI * E.x;
    float CosTheta = sqrt( (1 - E.y) / ( 1 + (m2 - 1) * E.y ) );
    float SinTheta = sqrt( 1 - CosTheta * CosTheta );

    vec3 H;
    H.x = SinTheta * cos( Phi );
    H.y = SinTheta * sin( Phi );
    H.z = CosTheta;
    
    float d = ( CosTheta * m2 - CosTheta ) * CosTheta + 1;
    float D = m2 / ( PI*d*d );
    float PDF = D * CosTheta;

    return vec4( normalize(H) , PDF );
}

float G1_smith(float roughness, float NoX){
    float k = Pow2(roughness) / 2.0; 
    return (NoX) / (NoX * (1.0 - k) + k);
}

float G_Smith(float roughness,float NoV,float NoL){
    return G1_smith(roughness,NoV) * G1_smith(roughness,NoL);
}

vec2 IntegrateBRDF(float Roughness,float NoV /*Cos ThetaV*/){
    vec3 V;
    V.x = sqrt(1. - NoV * NoV);
    V.y = 0.;    // simpliy
    V.z = NoV;
    float A = 0;
    float B = 0;
    const uint NumSamples = 1024;
    for( uint i = 0; i < NumSamples; i++ )
    {
        vec2 Xi = Hammersley( i, NumSamples);
        vec3 H = ImportanceSampleGGX( Xi,Pow4(Roughness)).xyz;
        vec3 L = 2 * dot( V, H ) * H- V;
        float NoL = Saturate( L.z );
        float NoH = Saturate( H.z );
        float VoH = Saturate( dot( V, H ) );
        if( NoL > 0 )
        {
            float G = G_Smith( Roughness, NoV, NoL );
            float G_Vis = G * VoH / (NoH * NoV);
            float Fc = pow( 1- VoH, 5 );
            A += (1- Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    return vec2( A, B ) / NumSamples;
}

void main(){
    ivec2 imgSize = imageSize(OutBrdfLUT);
    if(gl_GlobalInvocationID.x >= imgSize.x || gl_GlobalInvocationID.y >= imgSize.y) return;    float NoV       = max((float(gl_GlobalInvocationID.x) + 0.5) / float(imgSize.x), 0.001);
    float roughness = max((float(gl_GlobalInvocationID.y) + 0.5) / float(imgSize.y), 0.001);
    
    vec2 diffuse_specular = IntegrateBRDF(roughness, NoV);
    imageStore(OutBrdfLUT,ivec2(gl_GlobalInvocationID.xy),vec4(diffuse_specular,0.,1.));
}