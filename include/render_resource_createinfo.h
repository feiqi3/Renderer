#ifndef RENDER_RESOURCE_CREATE_INFO
#define RENDER_RESOURCE_CREATE_INFO
#include "render_resource.h"
namespace Render {
    struct BufferDesc {
        size_t byteSize;
        uint32_t bufUsage;
        MemoryUsage memUsage;
        uint8_t queueType;
        bool mappable = false;
    };
};

#endif