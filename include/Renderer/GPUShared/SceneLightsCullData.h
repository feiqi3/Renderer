#ifndef SCENE_LIGHTS_CULL_DATA_H_
#define SCENE_LIGHTS_CULL_DATA_H_

#include "GPUSharedDef.h"
#include "SceneLightsDef.h"
#include "SceneData.h"
#ifndef LIGHT_PER_FROXEL
#define LIGHT_PER_FROXEL 36
#endif

GPU_SHARED_NAMESPACE_BEGIN

    GPU_STRUCT_BEGIN(AABB)
        vec3 minP;
        vec3 maxP;
    GPU_STRUCT_END

    GPU_STRUCT_BEGIN(LightCullData)
        vec4 aabbInViewMin; // xyz: view min point, w: unused
        vec4 aabbInViewMax; // xyz: view max point, w: lightIndex
    GPU_STRUCT_END

    GPU_STRUCT_BEGIN(FroxelInfo)
        vec4 leftNormal;   // xyz: normal, w: unused
        vec4 rightNormal;  // xyz: normal, w: unused
        vec4 topNormal;    // xyz: normal, w: unused
        vec4 bottomNormal; // xyz: normal, w: unused
        float zNear;       // positive near
        float zFar;        // positive far
        float padding0;
        float padding1;
    GPU_STRUCT_END


    GPU_STRUCT_BEGIN(FroxelLightDataList)
        int lightCount;
        //This struct contains scalars only, so its GLSL std430 array stride is the
        //raw member size (148B) while the C++ sizeof is rounded up to 16B multiples
        //by GPU_SHARED_ALIGN(alignas(16)) -> 160B. The explicit padding below makes
        //both sides agree on 160B. See the note in GPUSharedDef.h for when padding is needed.
        int padding0;
        int padding1;
        int padding2;
        int lightIndex[LIGHT_PER_FROXEL];
    GPU_STRUCT_END

    GPU_STRUCT_BEGIN(ClusterInfo)
        int MaxTileX;
        int MaxTileY;
        int MaxTileZ;
        float specialNear;
        float near;
        float far;
        vec2  screenXY;
    GPU_STRUCT_END
GPU_SHARED_NAMESPACE_END


#ifndef __cplusplus

void _GetAABBCorners(AABB aabb, out vec3 corners[8]) {
    corners[0] = vec3(aabb.minP.x, aabb.minP.y, aabb.minP.z);
    corners[1] = vec3(aabb.maxP.x, aabb.minP.y, aabb.minP.z);
    corners[2] = vec3(aabb.minP.x, aabb.maxP.y, aabb.minP.z);
    corners[3] = vec3(aabb.maxP.x, aabb.maxP.y, aabb.minP.z);
    corners[4] = vec3(aabb.minP.x, aabb.minP.y, aabb.maxP.z);
    corners[5] = vec3(aabb.maxP.x, aabb.minP.y, aabb.maxP.z);
    corners[6] = vec3(aabb.minP.x, aabb.maxP.y, aabb.maxP.z);
    corners[7] = vec3(aabb.maxP.x, aabb.maxP.y, aabb.maxP.z);
}

void GetAABBCorners(LightCullData cullData, out vec3 corners[8]) {
    AABB aabb;
    aabb.minP = cullData.aabbInViewMin.xyz;
    aabb.maxP = cullData.aabbInViewMax.xyz;
    _GetAABBCorners(aabb, corners);
}

#endif //!__cplusplus

#endif // SCENE_LIGHTS_CULL_DATA_H_