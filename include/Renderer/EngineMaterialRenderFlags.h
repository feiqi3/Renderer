#ifndef ENGINE_ENTITY_FILTER_MASK_H_
#define ENGINE_ENTITY_FILTER_MASK_H_
#include "common/CoreDefs.h"

namespace Render {
	namespace EntityTagMask {
		u64 All = 0xFFFFFFFFFFFFFFFF;
		u64 ShadowCaster = 0x0000000000000010;
	};
}

#endif