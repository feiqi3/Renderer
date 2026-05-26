#ifndef ENGINE_ENTITY_FILTER_MASK_H_
#define ENGINE_ENTITY_FILTER_MASK_H_
#include "common/CoreDefs.h"

namespace Render {
	namespace CullMask {
		inline u32 Default			= 1		<<	0;
		inline u32 ShadowOnly		= 1		<<	1;

		inline u32 All				= 0xFFFFFFFF;
	};
}

#endif