#ifndef ENGINE_PASS_H_
#define ENGINE_PASS_H_
#include "common/Name.h"

namespace Render {
	namespace PassName {
#define ENGINE_PASS_DEFINITION(PASS_NAME) inline const Name PASS_NAME = Name( #PASS_NAME );
#include "EnginePassItems.h"
#undef ENGINE_PASS_DEFINITION
	}
}

#endif