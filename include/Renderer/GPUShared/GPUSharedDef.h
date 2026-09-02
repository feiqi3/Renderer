#ifndef GPUSHARED_DEF_H_
#define GPUSHARED_DEF_H_

#ifdef __cplusplus
#include "common/CommonMath.h"
#endif

#define VAR_ARR_SIZE  1

#ifdef __cplusplus
#define GPU_SHARED_NAMESPACE_BEGIN namespace Render::GPUShared {
#define GPU_SHARED_NAMESPACE_END }
#define GPU_SHARED_ALIGN alignas(16)
using   uint = uint32_t;
#else
#define GPU_SHARED_NAMESPACE_BEGIN
#define GPU_SHARED_NAMESPACE_END
#define GPU_SHARED_ALIGN
#endif

//NOTE ON PADDING:
//GPU_SHARED_ALIGN(alignas(16)) rounds the C++ sizeof of every GPU struct up to a
//multiple of 16, which matches the std430 layout of structs containing vec4/mat4
//members (they are naturally 16-byte aligned).
//However, structs made of SCALARS ONLY (int/float/bool, alignment 4) are packed by
//GLSL std430 with NO 16-byte rounding: their array stride is just the raw member
//size sum, which is smaller than the C++ sizeof. Such structs MUST add explicit
//trailing padding members (padding0/padding1/...) so both sides agree.
//Example: 1 int + int[36] = 148B raw, C++ sizeof = 160B -> add 3 int padding.
#define GPU_STRUCT_BEGIN(name) struct GPU_SHARED_ALIGN name {
#define GPU_STRUCT_END };

#endif // GPUSHARED_DEF_H_