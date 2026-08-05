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

#define GPU_STRUCT_BEGIN(name) struct GPU_SHARED_ALIGN name {
#define GPU_STRUCT_END };

#endif // GPUSHARED_DEF_H_