#version 450
#include "CommonSets.inl"
layout(set = 1, binding = 0) uniform UniformBufferObject {
	mat4 MatTransform;
} ObjectCommon;

layout(set = 1, binding = 1) uniform texture2D BoxTex;
layout(set = 1, binding = 2) uniform sampler  BoxSampler;

layout(location = 0) in vec3 inPosition;   // pos0
layout(location = 1) in vec3 inNormal;     // pos1
layout(location = 2) in vec2 inTexCoord;   // pos2

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 worldPos;

void main()
{
    fragTexCoord = inTexCoord;
    worldPos = (ObjectCommon.MatTransform * vec4(inPosition,1.0f)).xyz;
    fragNormal =  (mat4(mat3(ObjectCommon.MatTransform)) * vec4(inNormal,1.0f)).xyz;
    gl_Position = CameraCommon.camera.MatProj * CameraCommon.camera.MatView * ObjectCommon.MatTransform * vec4(inPosition, 1.0);
}