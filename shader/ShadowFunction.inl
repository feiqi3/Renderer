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

float calculateShadowCSM(vec4 worldSpacePos, vec4 worldNormal)
{
    GPUDirLightShadowData dirShadowData = SHADOWDATA.DirLightShadowInfo;
    vec3 lightDir = normalize(dirShadowData.LightDir.xyz);

    // 1. Calculate lighting angles
    float cosNL = dot(lightDir, worldNormal.xyz);
    if (cosNL < 0.0) return 0.0; // Early exit for back-facing surfaces

    float sinNL = sqrt(max(0.0, 1.0 - cosNL * cosNL));
    float safeCosNL = max(cosNL, 0.001);
    float tanNL = sinNL / safeCosNL;

    vec2 shadowMapSize = dirShadowData.AtlasInfo.xy;
    int shadowTechnique = int(SHADOWDATA.ShadowInfo.y);
    int cascadedLayers = int(dirShadowData.AtlasInfo.z);
    float interpolateFactor = dirShadowData.AtlasInfo.w;

    // 2. Find which cascade layer the world position belongs to
    float depthInMainCamSpace = abs((CAMDATA.MatView * worldSpacePos).z);
    int layerIdx = -1;
    for (int i = cascadedLayers - 1; i >= 0; --i)
    {
        float zEnd = dirShadowData.cascadedShadowData[i].Info.x;
        if (depthInMainCamSpace < zEnd)
        {
            layerIdx = i;
        }
        else
        {
            break;            
        }
    }

    // Out of shadow range
    if (layerIdx == -1)
    {
        return 1.0;
    }

    // 3. Compute smooth transition factor between cascades
    bool hasSmoothTransition = false;
    float interpolationFactor = 1.0;
    if (layerIdx < cascadedLayers)
    {
        float zEnd = dirShadowData.cascadedShadowData[layerIdx].Info.x;
        float zLastLayerEnd = 0.0;
        if (layerIdx > 0)
        {
            zLastLayerEnd = dirShadowData.cascadedShadowData[layerIdx - 1].Info.x;
        }
        float zArea = zEnd - zLastLayerEnd;
        
        // Compute linear interpolation ratio across the overlap margin
        interpolationFactor = (zEnd - depthInMainCamSpace) / (zArea * interpolateFactor);
        if (interpolationFactor >= 0.0 && interpolationFactor <= 1.0)
        {
            hasSmoothTransition = true;
        }
        interpolationFactor = clamp(interpolationFactor, 0.0, 1.0);
    }

    // 4. Calculate Normal Offset Bias using OrthoSize (Prevents acne when light is vertical)
    float orthoWidthCur = dirShadowData.cascadedShadowData[layerIdx].OrthoSize.x;
    float texelSizeWorldCur = orthoWidthCur / shadowMapSize.x;
    float normalOffsetScale = 0.5; // Scaled offset magnitude
    vec3 offsetWorldPosCur = worldSpacePos.xyz + worldNormal.xyz * (sinNL * normalOffsetScale * texelSizeWorldCur);

    // Depth Bias: Base bias + slope-scaled bias clamped to prevent Peter Panning
    float baseBias = 0.00025;
    float maxBias = 0.0005;
    float bias = clamp(baseBias + 0.001 * tanNL, baseBias, maxBias);

    // 5. Transform offset position to Current Layer Shadow Space
    mat4 shadowViewProjMat = dirShadowData.cascadedShadowData[layerIdx].ProjMat * dirShadowData.cascadedShadowData[layerIdx].ViewMat;
    vec4 shadowNDC = shadowViewProjMat * vec4(offsetWorldPosCur, 1.0);
    shadowNDC /= shadowNDC.w;
    vec2 shadowUV = mapShadowUV(shadowNDC.xy);
    float depthInLightSpaceCurLayer = shadowNDC.z;

    // 6. Transform offset position to Next Layer Shadow Space (for smooth blending)
    vec2 nextShadowUV = vec2(0.0);
    float depthInLightSpaceNextLayer = 1.0;
    bool hasNextLayer = false;

    if (layerIdx < cascadedLayers - 1)
    {
        float orthoWidthNext = dirShadowData.cascadedShadowData[layerIdx + 1].OrthoSize.x;
        float texelSizeWorldNext = orthoWidthNext / shadowMapSize.x;
        vec3 offsetWorldPosNext = worldSpacePos.xyz + worldNormal.xyz * (sinNL * normalOffsetScale * texelSizeWorldNext);

        mat4 nextShadowViewProjMat = dirShadowData.cascadedShadowData[layerIdx + 1].ProjMat * dirShadowData.cascadedShadowData[layerIdx + 1].ViewMat;
        vec4 nextShadowNDC = nextShadowViewProjMat * vec4(offsetWorldPosNext, 1.0);
        nextShadowNDC /= nextShadowNDC.w;
        nextShadowUV = mapShadowUV(nextShadowNDC.xy);
        hasNextLayer = isShadowUVInRange(nextShadowUV);
        depthInLightSpaceNextLayer = nextShadowNDC.z;
    }

    // 7. Calculate Shadow Visibility based on technique
    float shadowVis = 1.0;
    float nextShadowVis = 1.0;

    if (shadowTechnique == 0) // Hard Shadow
    {
        float sampledDepthInCurLayer = texture(sampler2DArray(DirShadowMap, ShadowSampler), vec3(shadowUV, layerIdx)).r;
        shadowVis = (depthInLightSpaceCurLayer - bias > sampledDepthInCurLayer) ? 0.0 : 1.0;

        if (hasSmoothTransition && hasNextLayer)
        {
            float sampledDepthInNextLayer = texture(sampler2DArray(DirShadowMap, ShadowSampler), vec3(nextShadowUV, layerIdx + 1)).r;
            nextShadowVis = (depthInLightSpaceNextLayer - bias > sampledDepthInNextLayer) ? 0.0 : 1.0;
        }
        else
        {
            interpolationFactor = 1.0;
        }
    }
    else if (shadowTechnique == 1) // PCF Filtered Shadow
    {
        shadowVis = sampleDirectionLightPCFArrayLayer(
            DirShadowMap, ShadowSampler, shadowUV, float(layerIdx), shadowMapSize, depthInLightSpaceCurLayer, bias
        );

        if (hasSmoothTransition && hasNextLayer)
        {
            nextShadowVis = sampleDirectionLightPCFArrayLayer(
                DirShadowMap, ShadowSampler, nextShadowUV, float(layerIdx + 1), shadowMapSize, depthInLightSpaceNextLayer, bias
            );
        }
        else
        {
            interpolationFactor = 1.0;
        }
    }

    // 8. Interpolate between current layer and next layer
    return shadowVis * interpolationFactor + nextShadowVis * (1.0 - interpolationFactor);
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