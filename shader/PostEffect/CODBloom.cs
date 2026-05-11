#version 450
#include "../BindlessSet.inl"
#include "../ShaderResource.inl"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

//Next-Generation-Post-Processing-in-Call-of-Duty-Advanced-Warfare
DECL_BUFFER_STD430_BEG(BloomConfig)
    float BloomStrength;
    float Threshold;        
    float Radius;               //Up sample parameter, controls the uv step. 
    float Karis;                //Do Karis? alway in downsample mip0 -> mip1
DECL_BUFFER_STD430_BEG

RESOURCE_DECL_BEG(1)
#ifdef DOWN_SAMPLE
    //mip x
    SLOT_RWTEXTURE(1, image2D, MipN)
    //mip x - 1
    SLOT_TEXTURE(1, texture2D, MipN_1)
#else
    //mip x --> read
    SLOT_TEXTURE(1, texture2D, MipN)
    //mip x - 1 --> write
    SLOT_RWTEXTURE(1, image2D, MipN_1)
#endif
    SLOT_SAMPLER(1, sampler, BilinearSampler)
    SLOT_CONST_BUFFER(1,BloomConfig,Cfg)
RESOURCE_DECL_END

// struct BloomConfig{
//     float BloomStrength;
//     float Threshold;        
//     float Karis;       
//     float Padding3;
// };

// layout(set = 1, binding = 0) uniform UBOBlock {
//     BloomConfig _;
// }cfg;
#define BLOOM_CFG GetBuffer(Cfg)

float Luminance(vec3 rgb){
    return dot(rgb , vec3(0.484375,0.515625,0.34375));
}

vec3 KarisAverage(vec3 rgb){
    float weight = 1. / (1. + Luminance(rgb));
    return vec3(weight) * rgb;
}


const ivec2 Tap13SamplePointsOffset[] = {
    ivec2(-2,-2),ivec2( 0,-2),ivec2( 2,-2),
        ivec2(-1,-1),ivec2( 1,-1),
    ivec2(-2,0 ),ivec2( 0, 0),ivec2( 2, 0),
        ivec2(-1, 1),ivec2( 1, 1),
    ivec2(-2,2 ),ivec2( 0, 2),ivec2( 2, 2)
};



const int Tap13Blocks[] = {
    0, 1, 3, 5, 6, //Block0
    1, 2, 4, 6, 7, //Block1
    5, 6, 8,10,11, //Block2
    6, 7, 9,11,12  //Block3 
};

const float Tap13FilterFactor[] = {
    0.125,  0.125,  0.5,    0.125,  0.125    
};

const float tentFilter[] = {
                            1., 2., 1.,
                            2., 4., 2.,
                            1., 2., 1.
                        };
const ivec2 tentFilterOffset[] = {
    ivec2(-1,-1),    ivec2( 0,-1),    ivec2( 1,-1),
    ivec2(-1, 0),    ivec2( 0, 0),    ivec2( 1, 0),
    ivec2(-1, 1),    ivec2( 0, 1),    ivec2( 1, 1)
};

vec3 TentFilterSample(vec2 uv, vec2 uvStepPerPixSmallTex , texture2D sampledTexture , sampler samp){
    float radius = max(0.0001,BLOOM_CFG.Radius);
    vec2 uvStep = uvStepPerPixSmallTex * radius;
    vec3 outCol = vec3(0.);
    for(int i = 0; i < 9; ++ i){
        vec2 uvSample = uv + vec2(tentFilterOffset[i]) * uvStep;
        vec3 col = texture(
            Sampler2D(sampledTexture,samp), uvSample
        ).rgb;
        col = col * 0.0625 * vec3(tentFilter[i]);
        outCol += col;
    }
    return out;
}

vec3 Tap13Sample(vec2 curUV,vec2 uvStep,texture2D sampledTexture,sampler textureSampler){
    vec3 tap13SampledPoint[13];
    for(int i = 0;i < 13; ++i){
        vec2 uvOffset = vec2(Tap13SamplePointsOffset[i]) * uvStep + curUV;
        tap13SampledPoint[i] = texture(Sampler2D(sampledTexture,textureSampler), uvOffset).rgb;
    }

    vec3 blockColor[4];

    for(int i = 0;i < 4; ++i){
        int blockOffsetBeg = i * 5;
        blockColor[i] = 
            vec3(Tap13FilterFactor[0]) * tap13SampledPoint[ Tap13Blocks[blockOffsetBeg + 0] ] + 
            vec3(Tap13FilterFactor[1]) * tap13SampledPoint[ Tap13Blocks[blockOffsetBeg + 1] ] + 
            vec3(Tap13FilterFactor[2]) * tap13SampledPoint[ Tap13Blocks[blockOffsetBeg + 2] ] + 
            vec3(Tap13FilterFactor[3]) * tap13SampledPoint[ Tap13Blocks[blockOffsetBeg + 3] ] + 
            vec3(Tap13FilterFactor[4]) * tap13SampledPoint[ Tap13Blocks[blockOffsetBeg + 4] ];
    }
    vec3 ret = vec3(0.);
    for(int i = 0;i < 4; ++i){
        if(BLOOM_CFG.Karis > 0){
            blockColor[i] = KarisAverage(blockColor[i]);
        }
        ret += blockColor[i] * vec3(0.25) ;
    }

    return ret;
}

void DownSampleMain(){
    ivec2 imgSize = imageSize(GetRWTexture(MipN));
    if(gl_GlobalInvocationID.x >= imgSize.x || gl_GlobalInvocationID.y >= imgSize.y){
        return;
    }
    ivec2 curPixel = gl_GlobalInvocationID.xy;
    ivec2 sampledImgSize = textureSize(GetTexture(MipN_1));
    vec2 uvStep = vec2(1.) / sampledImgSize;
    vec2 curUV  = ((vec2(0.5) + vec2(curPixel) )/ vec2(imgSize));
    vec3 tap13SampledOutcome = Tap13Sample(curUV , uvStep, GetTexture(MipN_1), GetSampler(BilinearSampler));
    imageStore(GetRWTexture(MipN), curPixel, vec4(tap13SampledOutcome,1.));
}

void UpSampleMain(){
    ivec2 imgSize = imageSize(GetRWTexture(MipN_1));
    if(gl_GlobalInvocationID.x >= imgSize.x || gl_GlobalInvocationID.y >= imgSize.y){
        return;
    }
    ivec2 mipnSize = textureSize(GetTexture(MipN));
    ivec2 curPixel = gl_GlobalInvocationID.xy;
    vec3 curPixCol = imageLoad(GetRWTexture(MipN_1), curPixel).rgb;
    vec2 curUV = (vec2(0.5) + vec2(curPixel)) / vec2(imgSize);
    vec2 uvStepPerPixMipN = vec2(1.) / vec2(mipnSize);
    vec3 tentOut = TentFilterSample(curUV, uvStepPerPixMipN, GetTexture(MipN), GetSampler(BilinearSampler));
    vec4 imgOut  = vec4(tentOut + curPixCol,1.);
    imageStore(GetRWTexture(MipN_1),curPixel, imgOut);
}