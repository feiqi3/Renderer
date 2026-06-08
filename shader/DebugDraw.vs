#version 450

#include "CommonSets.inl"

layout(location = 0) in vec3 i_pos;         
   
layout(location = 1) in mat4 i_perObjWorld;            
layout(location = 2) in vec4 i_perObjColor;            

layout(location = 0) out vec3 o_color;

void main(){
    vec4 worldPos =  (i_perObjWorld) * vec4(i_pos,1.f);
    o_color = i_perObjColor.xyz;
    gl_Position = CAMDATA.MatViewProj * worldPos;
}