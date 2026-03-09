#ifndef COMPONENTSYSTEM_H_
#define COMPONENTSYSTEM_H_
#include <functional>
#include <memory>
#include <mutex>

#include "function/ComponentFwd.h"

#include "common/Singleton.h"
#include "function/ObjectFwd.h"


class Component;

namespace Render { 
	class ComponentSystem :public Singleton<ComponentSystem>{
	public:
		ComponentSystem() = default;
		~ComponentSystem();

	public:
		void delegateDestroyComponent(ComponentUniquePtr& comp);
		
	public:
		void doDestroyComponents();

	private:
		void destroyComponent(ComponentUniquePtr& comp);

	private:
		std::vector<ComponentUniquePtr> mDelegateDestroyComponents;
		std::mutex mDestroyComponentDelegateMutex;

	};
}

#endif // COMPONENTSYSTEM_H_