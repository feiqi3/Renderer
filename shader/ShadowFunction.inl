#define SHADOW_BIAS 0.0025f

#if !defined(SHADOW_PASS)

float calculateShadowVisFactor(vec4 worldSpacePos)
{
    float shadowVis = 1.0f;



    //1. Dir light shadow
    {
        vec4 worldPos = worldSpacePos;
        mat4 dirShadowViewProjMat = SHADOWDATA.DirLightShadowInfo.ProjMat * SHADOWDATA.DirLightShadowInfo.ViewMat;
        vec4 shadowPosNDC   = dirShadowViewProjMat * worldPos;
        shadowPosNDC        = shadowPosNDC / shadowPosNDC.w;// x,y ~ [-1 , 1], z ~ [0 , 1]
        shadowPosNDC.xy     = shadowPosNDC.xy * 0.5 + 0.5;
        

        vec4 ndcPos = dirShadowViewProjMat * worldPos;
        ndcPos = ndcPos / ndcPos.w;
        //Depth in dirlight shadow camera
        float shadowMapDepth = texture(sampler2D(DirShadowMap, ShadowSampler), shadowPosNDC.xy).r;
        //depth from world space to dirlight shadow camera
        float depth = ndcPos.z;
        depth = min(1.,depth);
        if(depth - SHADOW_BIAS < shadowMapDepth)
        {
            shadowVis = 1.0f;
        }
        else
        {
            shadowVis = 0.0f;
        }
        
        //Not in shadow map range
        if(shadowPosNDC.x < 0 || shadowPosNDC.x > 1 || shadowPosNDC.y < 0 || shadowPosNDC.y > 1)
        {
            shadowVis = 1.0f;
        }

    }

    //TODO: Point light shadow....
    return shadowVis;
}
#endif //!defined(SHADOW_PASS)