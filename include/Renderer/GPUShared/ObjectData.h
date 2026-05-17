#ifndef OBJECT_DATA_H_
#define OBJECT_DATA_H_

GPU_SHARED_NAMESPACE_BEGIN
	GPU_STRUCT_BEGIN(ObjectCommonData)
		mat4 worldMatrix;
		mat4 tansInvWorldMatrix;
	GPU_STRUCT_END
GPU_SHARED_NAMESPACE_END

#endif
