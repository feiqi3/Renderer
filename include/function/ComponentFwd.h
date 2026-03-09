#ifndef COMPONENT_FWD_H_
#define COMPONENT_FWD_H_

#include <memory>

namespace Render {
	class Component;
	using ComponentUniquePtr = std::unique_ptr<Component>;
}

#endif //COMPONENT_FWD_H_