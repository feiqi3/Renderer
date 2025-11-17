#version 450

layout(location = 0) in vec3 inPosition;   // pos0
layout(location = 1) in vec3 inNormal;     // pos1
layout(location = 2) in vec2 inTexCoord;   // pos2

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;

void main()
{
    gl_Position = vec4(inPosition / 5.0, 1.0);

    fragNormal = inNormal;
    fragTexCoord = inTexCoord;
}