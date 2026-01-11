#ifndef ENGINE_RESOURCE_MANAGER_REGISTER_H_
#define ENGINE_RESOURCE_MANAGER_REGISTER_H_
#include "common/Name.h"
namespace Render {
	void RegisterAllEngineResourceManager();
	void CreateAllPersistentResource();
	void UnRegisterAllEngineResourceManager();
}

#endif