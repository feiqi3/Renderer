#include "function/ComponentSystem.h"
#include "function/Component.h"

namespace Render {
	
	
	ComponentSystem::~ComponentSystem()
	{
		doDestroyComponents();
	}

	void ComponentSystem::delegateDestroyComponent(ComponentUniquePtr& comp)
	{
		if (comp->enabled()) {
			comp->setEnabled(false);
			comp->onDisable();
		}
		comp->setOwner(nullptr);
		comp->onDetach();
		mDelegateDestroyComponents.push_back(std::move(comp));
	}

	void ComponentSystem::doDestroyComponents()
	{
		for (auto&& comp : mDelegateDestroyComponents) {
			destroyComponent(comp);
		}
		mDelegateDestroyComponents.clear();
	}

	void ComponentSystem::destroyComponent(ComponentUniquePtr& comp)
	{
		comp->onDestroy();
		comp = nullptr;
	}

};