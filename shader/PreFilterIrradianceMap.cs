#version 450
#extension GL_EXT_shader_image_load_formatted : require

#include "IBLGenCOnfig.h"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(set = 0,binding = 0) uniform textureCube EnvCubeMap;
layout(set = 0,binding = 1) uniform sampler EnvCubeMapSampler;
layout(set = 0,binding = 2) uniform writeonly imageCube OutPrefilterEnvCubeMap;

layout(set = 0,binding = 3) uniform UBOBlock{
    PrefilterEnvMapCfg cfg;
}PrefilterCfg;

#include "CommonMath.inl"




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

    return vec4( normalize(H), PDF );
}

vec3 PrefilterEnvMap( uvec2 Random, float Roughness, vec3 R )
{
	vec3 FilteredColor = vec3(0.);
	float Weight = 0;
		
	const uint NumSamples = 1024;
	for( uint i = 0; i < NumSamples; i++ )
	{
		vec2 E = HammersleyRandomized( i, NumSamples, Random);
        vec3 H = TangentToWorld( ImportanceSampleGGX( E, Pow4(Roughness) ).xyz, R );
		vec3 L = 2 * dot( R, H ) * H - R;
        const float HDRSampleMax = 15;
		float NoL = Saturate( dot( R, L ) );
		if( NoL > 0 )
		{
		    vec3 sampleValue = textureLod( samplerCube(EnvCubeMap,EnvCubeMapSampler), L, 0. ).rgb;
			//avoid some extreme value.....   
            sampleValue = clamp(sampleValue,0.,HDRSampleMax);
            FilteredColor += sampleValue * NoL;
			Weight += NoL;
		}
	}

	return FilteredColor / max( Weight, 0.001 );
}

vec3 TexelToCubeMapDir(ivec2 texelCoord, int faceIndex, ivec2 faceSize) 
{
    vec2 uv = (vec2(texelCoord) + 0.5);
    uv.x /= float(faceSize.x);
    uv.y /= float(faceSize.y);
    
    uv = uv * 2.0 - 1.0;
    
    vec3 dir;
    
    switch (faceIndex) 
    {
        case 0: dir = vec3( 1.0, -uv.y, -uv.x); break; // +X (Right)
        case 1: dir = vec3(-1.0, -uv.y,  uv.x); break; // -X (Left)
        case 2: dir = vec3( uv.x,  1.0,  uv.y); break; // +Y (Top)
        case 3: dir = vec3( uv.x, -1.0, -uv.y); break; // -Y (Bottom)
        case 4: dir = vec3( uv.x, -uv.y,  1.0); break; // +Z (Front)
        case 5: dir = vec3(-uv.x, -uv.y, -1.0); break; // -Z (Back)
        
        default: dir = vec3(0.0); break; 
    }
    
    return normalize(dir);
}

void main() {
    ivec2 imgSizeOut = imageSize(OutPrefilterEnvCubeMap);
    
    if (gl_GlobalInvocationID.x >= imgSizeOut.x || 
        gl_GlobalInvocationID.y >= imgSizeOut.y || 
        gl_GlobalInvocationID.z >= 6) {
        return;
    }

    vec3 dir = TexelToCubeMapDir(ivec2(gl_GlobalInvocationID.xy), int(gl_GlobalInvocationID.z), imgSizeOut);
    
    float roughness = max(0.001, PrefilterCfg.cfg.curRoughness);
    vec3 filterRGB = PrefilterEnvMap(hash_PCG23(uvec3(gl_GlobalInvocationID)), roughness, dir);

    ivec3 savePix = ivec3(gl_GlobalInvocationID.xyz);
    
    imageStore(OutPrefilterEnvCubeMap, savePix, vec4(filterRGB, 1.0));
}