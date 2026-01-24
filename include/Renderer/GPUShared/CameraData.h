#ifndef CAMERA_DATA_H_
#define CAMERA_DATA_H_

#include "GPUSharedDef.h"
GPU_SHARED_NAMESPACE_BEGIN

	GPU_STRUCT_BEGIN(GPUCameraData)
		mat4 MatView;
		mat4 MatProj;
		mat4 MatViewProj;
		mat4 MatInvView;
		mat4 MatInvProj;
		vec4 CameraPosition;
		vec4 CameraUp;
		vec4 CameraFront;
	GPU_STRUCT_END

GPU_SHARED_NAMESPACE_END
#endif //CAMERA_DATA_H_  