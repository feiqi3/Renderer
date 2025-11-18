layout(set = 0, binding = 0) uniform UniformBufferCamera {
	mat4 MatView;
	mat4 MatProj;
	mat4 MatViewProj;
	mat4 MatInvView;
	mat4 MatInvProj;
	vec4 CameraPosition;
	vec4 CameraUp;
	vec4 CameraFront;
} CameraCommon;