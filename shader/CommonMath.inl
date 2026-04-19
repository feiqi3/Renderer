vec3 RotateVector(vec3 v, vec4 q) {
    vec4 q_normalized = normalize(q);
    vec3 t = 2.0 * cross(q_normalized.xyz, v);
    return v + q_normalized.w * t + cross(q_normalized.xyz, t);
}

mat3 MatTangentToWorld(vec3 N)
{
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    
    return mat3(tangent, bitangent, N);
}

vec3 TangentToWorld(vec3 v, vec3 N){
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    return vec3(dot(tangent,N),dot(bitangent,N),dot(up,N));
}

float RadicalInverse_VdC_Scrambled(uint bits, uint scramble) 
{
    bits = bitfieldReverse(bits ^ scramble); 
    
    return float(bits) * 2.3283064365386963e-10; 
}

vec2 Hammersley(uint Index, uint NumSamples, uvec2 Random) 
{
    float e1 = fract(float(Index) / float(NumSamples) + float(Random.x & 0xffffu) * (1.0 / 65536.0));
    
    float e2 = RadicalInverse_VdC_Scrambled(Index, Random.y);
    
    return vec2(e1, e2);
}

const float PI = 3.141592653589793;

#define Saturate(x) clamp(x,0.,1.)

#define Pow2(x) x * x
#define Pow4(x) Pow2(x)*Pow2(x)