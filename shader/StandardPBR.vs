#version 450
#include "CommonSets.inl"
#include "PBREntity.h"

layout(set = 3, binding = 1) uniform PerMaterialObject {
    PBRData pbrData;
} CBUFFER_pbrData;

#define PBRDATA         CBUFFER_pbrData.pbrData
#define CAMDATA         CameraCommon.camera
#define LIGHTDATA       SceneCommon.sceneLights

layout(location = 0) in vec3 i_pos;         
layout(location = 1) in vec3 i_normal;           
layout(location = 2) in vec4 i_tangent;        
layout(location = 3) in vec2 i_texcoord0;        
layout(location = 4) in vec4 i_color;            

layout(location = 0) out vec3 o_worldPos;
layout(location = 1) out vec3 o_normal;
layout(location = 2) out vec3 o_tangent;
layout(location = 3) out vec3 o_bitangent;
layout(location = 4) out vec2 o_texcoord0;
layout(location = 5) out vec3 o_color;
layout(location = 6) out vec3 o_viewDir;



void main(){
    vec4 worldPos =  (ObjData.ObjData.worldMatrix) * vec4(i_pos,1.f);
    vec3 worldNormal = mat3(ObjData.ObjData.invWorldMatrix) * i_normal;
    vec3 viewDir = vec3(worldPos - CAMDATA.CameraPosition).xyz;
    o_worldPos = worldPos.xyz;
    o_normal = worldNormal;
    o_color = i_color.xyz;
    o_viewDir = viewDir;
    o_texcoord0 = i_texcoord0;

    o_tangent = mat3(ObjData.ObjData.worldMatrix) * i_tangent.xyz;
    o_bitangent = cross(worldNormal, o_tangent)* i_tangent.w;

    gl_Position = CAMDATA.MatViewProj * worldPos;
}