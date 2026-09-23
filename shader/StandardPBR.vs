#version 450
#include "CommonSets.inl"
#include "PBREntity.h"
#include "StandardPBR.inl"
#include "ShaderExtensions.inl"
#include "StandardPBR.inl"
#define CAMDATA         CameraCommon.camera
#define LIGHTDATA       SceneCommon.sceneLights

layout(location = 0) in vec3 i_pos;         
layout(location = 1) in vec3 i_normal;           
layout(location = 2) in vec4 i_tangent;        
layout(location = 3) in vec2 i_texcoord0;        
layout(location = 4) in vec4 i_color;            
#ifdef SKIN
layout(location = 5) in uvec4 i_jointIndice;        
layout(location = 6) in vec4 i_weights;    
#endif//SKIN
layout(location = 0) out vec3 o_worldPos;
layout(location = 1) out vec3 o_normal;
layout(location = 2) out vec3 o_tangent;
layout(location = 3) out vec3 o_bitangent;
layout(location = 4) out vec2 o_texcoord0;
layout(location = 5) out vec3 o_color;
layout(location = 6) out vec3 o_viewDir;


void main(){
#if defined(SKIN)
    mat4 finalSkinMat = mat4(1.0);
    int totalJointsNum = GetBuffer(u_anmInfoList).jointsNum;
    mat4 skinMat[4];
    skinMat[0] = mat4(1.0);
    skinMat[1] = mat4(1.0);
    skinMat[2] = mat4(1.0);
    skinMat[3] = mat4(1.0);

    if(totalJointsNum > 0){
        //Disable the skinning if data in Joint Buffer is invalid
        for(int i = 0;i < 4;++i){
            float blendWeight = i_weights[i];
            uint blendJoint  = i_jointIndice[i];
            if(blendJoint >= totalJointsNum){
                skinMat[i] = mat4(1.0);
                continue;
            }
            skinMat[i] = blendWeight * GetBuffer(u_anmInfoList).JointMulIBM[blendJoint];
        }
    }
    finalSkinMat = skinMat[0] + skinMat[1] + skinMat[2] + skinMat[3];
    vec4 worldPos =  (ObjData.ObjData.worldMatrix) * finalSkinMat * vec4(i_pos,1.f);
    vec3 worldNormal = mat3(ObjData.ObjData.tansInvWorldMatrix) * mat3(finalSkinMat) * i_normal;
#else
    vec4 worldPos =  (ObjData.ObjData.worldMatrix) * vec4(i_pos,1.f);
    vec3 worldNormal = mat3(ObjData.ObjData.tansInvWorldMatrix) * i_normal;

#endif//SKIN

    vec3 viewDir = vec3(worldPos - CAMDATA.CameraPosition).xyz;
    o_worldPos = worldPos.xyz;
    o_normal = normalize(worldNormal);
    o_color = i_color.xyz;
    o_viewDir = normalize(viewDir);
    o_texcoord0 = i_texcoord0;

    o_tangent = mat3(ObjData.ObjData.worldMatrix) * i_tangent.xyz;
    o_tangent = normalize(o_tangent);
    o_bitangent = cross(o_normal, o_tangent)* i_tangent.w;
    o_bitangent = normalize(o_bitangent);
    gl_Position = CAMDATA.MatViewProj * worldPos;
}