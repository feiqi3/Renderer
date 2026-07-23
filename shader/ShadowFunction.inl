#define SHADOW_BIAS 0.0025f

#if !defined(SHADOW_PASS)

const float SHADOW_PCF_FILTER[] = {
    0.25, 0.25, 0.25,
    0.25, 0.5, 0.25,
    0.25, 0.25, 0.25
};
const float FILTER_FACTOR = 1.0f / 2.5f;
const float FILTER_UV_SCALE = 1.f;


float sampleDirectionLightPCF(texture2D tex, sampler samp, vec2 uv,vec2 shadowMapSize, float depth)
{
    vec2 texSize = shadowMapSize;
    vec2  texelSize = vec2(1. / texSize.x, 1. / texSize.y);
    float shadowVis[9];
    for(int i = -1; i <= 1; ++i)
    {
        for(int j = -1; j <= 1; ++j)
        {
            vec2 offset = vec2(i, j) * texelSize * FILTER_UV_SCALE;
            //Depth in dirlight shadow camera
            float sampleDepth = texture(sampler2D(tex,samp), uv + offset).r;
            if(depth - SHADOW_BIAS < sampleDepth)
            {
                shadowVis[(i + 1) * 3 + (j + 1)] = 1.0f;
            }
            else
            {
                shadowVis[(i + 1) * 3 + (j + 1)] = 0.0f;
            }
        }
    }

    float result = 0.0f;
    for(int k = 0; k < 9; ++k)
    {
        result += shadowVis[k] * SHADOW_PCF_FILTER[k];
    }
    return result * FILTER_FACTOR;
}

float calculateShadowVisFactor(vec4 worldSpacePos)
{
    float shadowVis = 1.0f;
    if(SHADOWDATA.ShadowInfo.x < 0.f)
    {
        return 1.0f;
    }


    //1. Dir light shadow
    {
        vec4 worldPos = worldSpacePos;
        mat4 dirShadowViewProjMat = SHADOWDATA.DirLightShadowInfo.ProjMat * SHADOWDATA.DirLightShadowInfo.ViewMat;
        vec4 dirlightShadowUV   = dirShadowViewProjMat * worldPos;
        dirlightShadowUV        = dirlightShadowUV / dirlightShadowUV.w;// x,y ~ [-1 , 1], z ~ [0 , 1]
        dirlightShadowUV.xy     = dirlightShadowUV.xy * 0.5 + 0.5;
        vec2 shadowMapSize = SHADOWDATA.DirLightShadowInfo.AtlasInfo.xy;

        vec4 ndcPos = dirShadowViewProjMat * worldPos;
        ndcPos = ndcPos / ndcPos.w;
        //depth from world space to dirlight shadow camera
        float mainCamDepthInDirShadowSpace = ndcPos.z;
        mainCamDepthInDirShadowSpace = min(1.,mainCamDepthInDirShadowSpace);

        //Not in shadow map range
        if(dirlightShadowUV.x < 0 || dirlightShadowUV.x > 1 || dirlightShadowUV.y < 0 || dirlightShadowUV.y > 1)
        {
            return 1.0f;
        }

        int shadowTechnique = int(SHADOWDATA.ShadowInfo.y);
        if(shadowTechnique == 0)
        {
            //Normal shadow map, no filter
            float sampleDepth = texture(sampler2D(DirShadowMap, ShadowSampler), dirlightShadowUV.xy).r;
            if(mainCamDepthInDirShadowSpace - SHADOW_BIAS < sampleDepth)
            {
                shadowVis = 1.0f;
            }
            else
            {
                shadowVis = 0.0f;
            }
        }else if(shadowTechnique == 1){
            //PCF
            shadowVis = sampleDirectionLightPCF(DirShadowMap, ShadowSampler, dirlightShadowUV.xy, shadowMapSize, mainCamDepthInDirShadowSpace);
        }else{
            //??? visible
            return 1.0;
        }

    }

    //TODO: Point light shadow....
    return shadowVis;
}
#endif //!defined(SHADOW_PASS)