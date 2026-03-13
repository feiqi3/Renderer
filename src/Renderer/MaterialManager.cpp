#include "Renderer/MaterialManager.h"
#include "Renderer/MaterialTemplateManager.h"
#include <exception>
#include <stdexcept>
#include <cassert>

namespace Render {

    MaterialManager::MaterialManager() {}

    MaterialManager::~MaterialManager() {
        clearAll();
    }

    const Name& MaterialManager::typeName() const {
        return Material::typeName();
    }

    Material* MaterialManager::loadImpl(const Name& id) {
		throw std::runtime_error("MaterialManager::loadImpl not implemented.");
        return nullptr;
    }

    void MaterialManager::unloadImpl(Material* res) {
        if (res) {
            res->OnUnload();
            delete res;
        }
    }

	void MaterialManager::createNecessaryPersistenceResources()
	{
	}

}