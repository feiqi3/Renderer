#ifndef ENGINE_PASS_H_
#define ENGINE_PASS_H_
#include "common/Name.h"
namespace Render {
	namespace PassName {
		inline const Name MainCameraPass = Name("MainCameraPass");
		inline const Name SwapchainPass  = Name("SwapchainPass");
	}
}

#endif