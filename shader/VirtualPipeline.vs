#version 450
//This shader is used for shader descriptorSets reflection to create setlayout.
//Not really used for rendering
#include "CommonSets.inl"
layout(location = 0) out vec2 o_uv;
void main(){
	//Avoid compiler optimization
	o_uv = CameraCommon.camera.CameraUp.xy;
	gl_Position = vec4(1.,0.,0.,1.);
}