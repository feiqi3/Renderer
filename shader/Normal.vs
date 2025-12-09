#version 450
#include "CommonSets.inl"
layout(set = 1, binding = 0) uniform UniformBufferObject {
	mat4 MatTransform;
} ObjectCommon;

layout(location = 0) in vec3 inPosition;   // pos0
layout(location = 1) in vec3 inNormal;     // pos1
layout(location = 2) in vec2 inTexCoord;   // pos2

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;

void main()
{
    gl_Position = CameraCommon.MatProj * CameraCommon.MatView * ObjectCommon.MatTransform * vec4(inPosition, 1.0);
}