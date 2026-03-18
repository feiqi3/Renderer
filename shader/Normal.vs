#version 450
#include "CommonSets.inl"
layout(set = 3, binding = 1) uniform texture2D BoxTex;
layout(set = 3, binding = 2) uniform sampler  BoxSampler;

layout(location = 0) in vec3 inPosition;   // pos0
layout(location = 1) in vec3 inNormal;     // pos1
layout(location = 2) in vec2 inTexCoord;   // pos2

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 worldPos;

void main()
{
    fragTexCoord = inTexCoord;
    worldPos = (ObjData.ObjData.worldMatrix * vec4(inPosition,1.0f)).xyz;
    fragNormal =  (mat4(mat3(ObjData.ObjData.worldMatrix)) * vec4(inNormal,1.0f)).xyz;
    gl_Position = CameraCommon.camera.MatProj * CameraCommon.camera.MatView * ObjData.ObjData.worldMatrix * vec4(inPosition, 1.0);
}