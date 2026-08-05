vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}


#if !defined(NO_LIGHTS)
vec3 PrefilterEnvMap(vec3 R,float roughness){
    roughness = clamp(roughness,0,1);
    int mipCtrl = int(LIGHTDATA.IBLControl.x);
    float mipToUse = clamp(roughness * (mipCtrl - 1),0,(mipCtrl - 1));
    return textureLod(samplerCube(PreFilterEnvMap,SceneTextureSampler),R,mipToUse).rgb;
}

vec2 PrefilterBRDF(float roughness, float NoV){
    return textureLod(sampler2D(BRDFLut,SceneTextureSampler),vec2(NoV,roughness),0).rg;
}

vec3 CalcF0(vec3 albedo, float metallic){
    return mix(vec3(0.04),albedo,metallic);
}

vec3 IBLCalculate(vec3 V, vec3 N, vec3 F0, float roughness){
    //1. reflect dir
    //Currently V -> from object surface to camera
    vec3 R      = reflect(-V,N);// normalize(2 * dot(V,N) * N - V);
    float NoV   = Saturate( dot(N,V) );
    vec3 PrefilteredColor = PrefilterEnvMap(R,roughness).rgb;
    vec2 BRDF             = PrefilterBRDF(roughness,NoV) ;
    vec3 specular = PrefilteredColor * (F0 * BRDF.r + BRDF.g);
    return specular;
}
#endif//!NO_LIGHTS