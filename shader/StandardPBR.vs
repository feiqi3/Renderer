#version 450
#include "CommonSets.inl"
#include "PBREntity.h"

layout(set = 2, binding = 1) uniform UniformBufferObject {
    PBRData pbrData
} pbrData;

#define PBRDATA         pbrData.pbrData
#define CAMDATA         CameraCommon.camera
#define LIGHTDATA       SceneCommon.sceneLights

layout(location = 0) in vec3 i_pos;         
layout(location = 1) in vec3 i_normal;           
layout(location = 2) in vec2 i_texcoord0;        
layout(location = 3) in vec2 i_texcoord1;        
layout(location = 4) in vec4 i_color;            

layout(location = 0) out vec3 o_worldPos;
layout(location = 1) out vec3 o_normal;
layout(location = 2) out vec2 o_texcoord0;
layout(location = 3) out vec3 o_texcoord1;
layout(location = 4) out vec3 o_color;
layout(location = 5) out vec3 o_viewDir;



void main(){
    vec4 worldPos =  PBRDATA.worldMatrix * vec4(i_pos,1.f);
    vec3 worldNormal = mat3(PBRDATA.invWorldMatrix) * i_normal;
    vec3 viewDir = worldPos - CAMDATA.CameraPosition;
    o_viewDir = viewDir
    o_texcoord0 = i_texcoord0;
    o_texcoord1 = i_texcoord1;
    gl_Position = vec4(0.f,0.f,0.f,1.f);
}