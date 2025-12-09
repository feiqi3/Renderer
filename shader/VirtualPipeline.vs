#version 450
#include "CommonSets.inl"
layout(location = 0) out vec2 o_uv;
void main(){
	//Avoid compiler optimization
	o_uv = CameraCommon.CameraUp.xy;
	gl_Position = vec4(1.,0.,0.,1.);
}