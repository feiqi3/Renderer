#ifndef DRAW_META_H_
#define DRAW_META_H_

#include "GPUSharedDef.h"

//This struct is used to record some meta data in this drawcall.
GPU_SHARED_NAMESPACE_BEGIN

GPU_STRUCT_BEGIN(DrawMeta)
	uint drawcallIndex;
	uint pipelineIndex;
	uint indexNum;
	uint instanceNum;
	uint indexBaseOffset;
	uint vertexBaseOffset;
	uint64_t debugFlagBufferAddress;
GPU_STRUCT_END

GPU_STRUCT_BEGIN(ComputeMeta)
	uint dispatchIndex;
	uint pipelineIndex;
	uint dispatchX;
	uint dispatchY;
	uint dispatchZ;
	uint padding0;
	GPU_STRUCT_END

GPU_SHARED_NAMESPACE_END
#endif//DRAW_META_H_