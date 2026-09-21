#ifndef SHADER_DRAW_META_DATA_INL
#define SHADER_DRAW_META_DATA_INL

#if defined(DRAW_META_DATA)
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "DrawMeta.h"

#ifdef COMPUTE
layout(push_constant, std430) uniform metaData {
    ComputeMeta meta;
};
#define GET_DISPATCH_INDEX() meta.dispatchIndex
#define GET_PIPELINE_INDEX() meta.pipelineIndex
#define GET_DISPATCH_SIZE () ivec3(meta.dispatchX,meta.dispatchY,meta.dispatchZ)
#else
layout(push_constant, std430) uniform metaData {
    DrawMeta meta;
};
#define GET_DRAW_CALL_INDEX()   meta.drawcallIndex
#define GET_PIPELINE_INDEX()    meta.pipelineIndex
#define GET_INDEX_NUM()         meta.indexNum
#define GET_INSTANCE_NUM()      meta.instanceNum
//The RHI layer reuses the generic metadata slot (DrawMeta.debugFlagBufferAddress)
//to carry the debug-print flag buffer address, so the upper (non-RHI) layer
//neither allocates nor knows about that buffer.
#define GET_DEBUG_FLAG_BUFFER_ADDRESS()  meta.debugFlagBufferAddress
#define GET_INDEX_BASE_OFFSET() meta.indexBaseOffset
#define GET_VERTEX_BASE_OFFSET() meta.vertexBaseOffset
#endif //COMPUTE

#else

#ifdef COMPUTE
#define GET_DISPATCH_INDEX() 0
#define GET_PIPELINE_INDEX() 0
#define GET_DISPATCH_SIZE () ivec3(0,0,0)
#else
#define GET_DRAW_CALL_INDEX() 0
#define GET_PIPELINE_INDEX() 0
#define GET_INDEX_NUM() 0
#define GET_INSTANCE_NUM() 0
#define GET_DEBUG_FLAG_BUFFER_ADDRESS() 0
#define GET_INDEX_BASE_OFFSET() 0
#define GET_VERTEX_BASE_OFFSET() 0
#endif //COMPUTE


#endif//DRAW_META_DATA



#endif//SHADER_DRAW_META_DATA_INL