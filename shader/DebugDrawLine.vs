#version 450

#include "CommonSets.inl"
#include "CommonMath.inl"
layout(location = 0) in vec3 i_beg;
layout(location = 1) in vec3 i_end;
layout(location = 2) in vec4 i_color;
layout(location = 3) in float i_width;

layout(location = 0) out vec4 o_color;
layout(location = 1) out flat uint o_instanceID;

const vec2 QUAD_VERTICES[6] = vec2[](
    vec2(0.0,  1.0),
    vec2(0.0, -1.0),
    vec2(1.0,  1.0),

    vec2(1.0,  1.0), 
    vec2(0.0, -1.0), 
    vec2(1.0, -1.0) 
);

void main(){
    o_color = i_color;

    //Do it in screen space 
    mat4 MVP = CAMDATA.MatViewProj * OBJDATA.worldMatrix;
    vec4 ibegInScreen = MVP * vec4(i_beg,1.f);
    vec4 iendInScreen = MVP * vec4(i_end,1.f);
    vec2 dir = normalize(iendInScreen.xy - ibegInScreen.xy);
    vec2 normal = vec2(-dir.y, dir.x);
    vec2 offset = normal * i_width * 0.5;
    vec2 vertexConfig = QUAD_VERTICES[gl_VertexIndex % 6];
    vec4 targetPos = mix(ibegInScreen, iendInScreen, vertexConfig.x);

    o_instanceID = gl_InstanceIndex;

    gl_Position = targetPos + vec4(offset * vertexConfig.y * targetPos.w, 0.0, 0.0);
}