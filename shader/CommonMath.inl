vec3 RotateVector(vec3 v, vec4 q) {
    vec4 q_normalized = normalize(q);
    vec3 t = 2.0 * cross(q_normalized.xyz, v);
    return v + q_normalized.w * t + cross(q_normalized.xyz, t);
}

mat3 MatTangentToWorld(vec3 N)
{
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = normalize(cross(N, tangent));
    
    return mat3(tangent, bitangent, N);
}

vec3 TangentToWorld(vec3 v, vec3 N){
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    return v.x * tangent + v.y * bitangent + v.z * N;
}

float RadicalInverse_VdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    
    return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint i, uint N) 
{
    return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

float RadicalInverse_VdC_Scrambled(uint bits, uint scrambleSeed) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    
    bits ^= scrambleSeed;
    
    return float(bits) * 2.3283064365386963e-10;
}

vec2 HammersleyRandomized(uint i, uint N, uvec2 randomSeed) 
{
    float E1 = fract(float(i) / float(N) + float(randomSeed.x) * (1.0 / 65536.0));
    
    float E2 = RadicalInverse_VdC_Scrambled(i, randomSeed.y);
    
    return vec2(E1, E2);
}

uvec2 hash_PCG23(uvec3 v) 
{
    v = v * 1664525u + 1013904223u;
    
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    
    v ^= v >> 16u;
    
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    
    return uvec2(v.x, v.y);
}

vec2 hash_DaveHoskins23(vec3 p3)
{
    p3 = fract(p3 * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx + p3.yz) * p3.zy);
}

const float PI = 3.141592653589793;

#define Saturate(x) clamp(x,0.,1.)

#define Pow2(x) x * x
#define Pow4(x) Pow2(x)*Pow2(x)