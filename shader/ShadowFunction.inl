#define SHADOW_BIAS 0.0025f

#if !defined(SHADOW_PASS)

const float SHADOW_PCF_FILTER[] = {
    0.25, 0.25, 0.25,
    0.25, 0.5, 0.25,
    0.25, 0.25, 0.25
};
const float FILTER_FACTOR = 1.0f / 2.5f;
const float FILTER_UV_SCALE = 1.f;

bool isShadowUVInRange(vec2 uv){
    return uv.x >= 0 && uv.x <= 1 && uv.y >= 0 && uv.y <= 1;
}

vec2 mapShadowUV(vec2 NDCUV){
    return NDCUV * 0.5 + 0.5;
}

float sampleDirectionLightPCFArrayLayer(texture2DArray tex, sampler samp, vec2 uv,float layer,vec2 shadowMapSize, float depth, float bias)
{
    vec2 texSize = shadowMapSize;
    vec2  texelSize = vec2(1. / texSize.x, 1. / texSize.y);
    float totalVis = 0.;
    int counter = 0;
    for(int i = -1; i <= 1; ++i)
    {
        for(int j = -1; j <= 1; ++j)
        {
            vec2 offset = vec2(i, j) * texelSize * FILTER_UV_SCALE;
            vec2 uvoffsetted = offset + uv;
            //Depth in dirlight shadow camera
            if(isShadowUVInRange(uvoffsetted)){
                float sampleDepth = texture(sampler2DArray(tex,samp), vec3(uvoffsetted,layer)).r;
                if(depth - bias < sampleDepth)
                {
                    totalVis += 1. * SHADOW_PCF_FILTER[counter] * FILTER_FACTOR; 
                }
            }
            counter++;
        }
    }

    return totalVis;
}

float calculateShadowCSM(vec4 worldSpacePos,vec4 worldNormal){
    GPUDirLightShadowData dirShadowData = SHADOWDATA.DirLightShadowInfo;
    vec3 lightDir = normalize(dirShadowData.LightDir.xyz);

    float cosNL = dot(lightDir.xyz,worldNormal.xyz);
    if(cosNL < 0.f)return 0.;

    //Calc bias
    float sinNL = abs(sqrt(1.f - cosNL * cosNL));
    cosNL = clamp(cosNL,0.01,1.);
    float tanNL = sinNL / cosNL;
    float bias  = 0.01 * tanNL;

    int shadowTechnique = int(SHADOWDATA.ShadowInfo.y);

    int cascadedLayers = int(dirShadowData.AtlasInfo.z);
    float interpolateFactor = dirShadowData.AtlasInfo.w;
    vec2 shadowMapSize = dirShadowData.AtlasInfo.xy;
    
    float shadowVis = 1.0f;
    float depthInMainCamSpace = abs(CAMDATA.MatView * worldSpacePos).z;
    bool  hasSmoothTransition = false;
    float interpolationFactor = 1.;
    float smoothInterplationDist = dirShadowData.AtlasInfo.w;
    //Find which cascaded layer the world space pos is in
    int layerIdx = -1;
    float smoothTransitionFactor = 0.;
    for(int i = cascadedLayers - 1; i >= 0; --i)
    {
        float zEnd = dirShadowData.cascadedShadowData[i].Info.x;
        if(depthInMainCamSpace < zEnd)
        {
            layerIdx = i;
        }else{
            break;            
        }
    }

    //Out of shadow range
    if(layerIdx == -1)
    {
        return 1.0f;
    }

    if(layerIdx <= cascadedLayers - 1){
        float zEnd = dirShadowData.cascadedShadowData[layerIdx].Info.x;
        float zLastLayerEnd = 0.;
        if(layerIdx > 0)
        {
            zLastLayerEnd = dirShadowData.cascadedShadowData[layerIdx - 1].Info.x;
        }
        float zArea = zEnd - zLastLayerEnd;
        interpolationFactor = (zEnd - depthInMainCamSpace) / (zArea * interpolateFactor);
        if(interpolationFactor > 1 || interpolationFactor < 0){
            hasSmoothTransition = false;
        }else{
            hasSmoothTransition = true;
        }
        interpolationFactor = clamp(interpolationFactor,0.,1.);
    }

    //Get the shadow view proj mat for this layer
    mat4 shadowViewProjMat = dirShadowData.cascadedShadowData[layerIdx].ProjMat * dirShadowData.cascadedShadowData[layerIdx].ViewMat;
    vec4 shadowNDC = shadowViewProjMat * worldSpacePos;
    shadowNDC = shadowNDC / shadowNDC.w;
    vec2 shadowUV = mapShadowUV(shadowNDC.xy);

    //Get ShadowUV in next layer
    vec4 nextShadowNDC = vec4(0);
    vec2 nextShadowUV  = vec2(0);
    bool hasNextLayer = false;

    
    float depthInLightSpaceNextLayer  = 1.0; 
    if(layerIdx < cascadedLayers - 1)
    {
        mat4 nextShadowViewProjMat = dirShadowData.cascadedShadowData[layerIdx + 1].ProjMat * dirShadowData.cascadedShadowData[layerIdx + 1].ViewMat;
        nextShadowNDC = nextShadowViewProjMat * worldSpacePos;
        nextShadowNDC = nextShadowNDC / nextShadowNDC.w;
        nextShadowUV = mapShadowUV(nextShadowNDC.xy);
        hasNextLayer = isShadowUVInRange(nextShadowUV.xy);
        
        depthInLightSpaceNextLayer = nextShadowNDC.z;
    }

    vec4 posInLightSpaceCurLayer = (dirShadowData.cascadedShadowData[layerIdx].ProjMat * 
        dirShadowData.cascadedShadowData[layerIdx].ViewMat * worldSpacePos);
    float depthInLightSpaceCurLayer = posInLightSpaceCurLayer.z / posInLightSpaceCurLayer.w;


    float nextShadowVis = 1.;
    if(shadowTechnique == 0){
        float sampledDepthInCurLayer = texture(sampler2DArray(DirShadowMap, ShadowSampler), vec3(shadowUV.xy, layerIdx)).r;
        //Smooth transition
        if(hasSmoothTransition && hasNextLayer){
            float sampledDepthInNextLayer = texture(sampler2DArray(DirShadowMap, ShadowSampler), vec3(nextShadowUV.xy, layerIdx + 1)).r;
            if(depthInLightSpaceNextLayer - bias > sampledDepthInNextLayer){
                nextShadowVis = 0.0;
            }
        }else{
            interpolationFactor = 1.;
        }
        
        if(depthInLightSpaceCurLayer - bias > sampledDepthInCurLayer){
            shadowVis = 0.;
        }else{
            shadowVis = 1.;
        }

    }else if(shadowTechnique == 1){
        shadowVis = sampleDirectionLightPCFArrayLayer(
            DirShadowMap, ShadowSampler, shadowUV.xy,float(layerIdx), shadowMapSize, depthInLightSpaceCurLayer,bias
        );
        if(hasSmoothTransition){
            nextShadowVis = sampleDirectionLightPCFArrayLayer(
            DirShadowMap, ShadowSampler, nextShadowUV.xy,float(layerIdx + 1), shadowMapSize, depthInLightSpaceNextLayer,bias
            );          
        }else{
            interpolationFactor = 1.;
        }
    }
    //Blend
    return shadowVis * (interpolationFactor) + nextShadowVis * (1. - interpolationFactor);
}

float calculateShadowVisFactor(vec4 worldSpacePos,vec4 worldNormal)
{
    float shadowVis = 0.f;
    GPUDirLightShadowData dirShadowData = SHADOWDATA.DirLightShadowInfo;
    if(SHADOWDATA.ShadowInfo.x < 0.f)
    {
        return 1.0f;
    }


    //1. Dir light shadow
    {
        shadowVis = calculateShadowCSM(worldSpacePos, worldNormal);
    }

    //TODO: Point light shadow....
    return shadowVis;
}
#endif //!defined(SHADOW_PASS)