#version 450
#include "CommonSets.inl"
#include "CommonMath.inl"
#include "SkyBoxData.h"
layout(set = 3,binding = 0)uniform ObjectUniformData{
    SkyBoxData data;
}skyboxData;

#define SKYBOX skyboxData.data 

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec3 o_viewDir;
void main() {
    vec2 pos;
    if (gl_VertexIndex == 0) {
        pos = vec2(-1.0, -1.0);
    } else if (gl_VertexIndex == 1) {
        pos = vec2(3.0, -1.0);
    } else {
        pos = vec2(-1.0, 3.0);
    }

    //Calculate view dir in viewspace 
    //Avoid float exposion caused by z = 1.
    vec4 targetInView = CAMDATA.MatInvProj * vec4(pos, 1.0, 1.0);
    //We dont care about vector's length,so only take its xyz
    vec3 viewSpaceDir = targetInView.xyz;
    vec3 worldDir = ( CAMDATA.MatInvView * vec4(viewSpaceDir, 0.0)).xyz;
    vec3 viewDir = normalize(worldDir);
    viewDir = RotateVector(viewDir, SKYBOX.rotateQuat);
    o_viewDir = viewDir;
    v_uv = pos * 0.5 + 0.5;
    gl_Position = vec4(pos, 1.0, 1.0);
}