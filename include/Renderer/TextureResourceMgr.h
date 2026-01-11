#ifndef TEXTURE_RESOURCE_MANAGER_H_
#define TEXTURE_RESOURCE_MANAGER_H_
#include "Texture.h"
#include "common/ResourceManager.h"
namespace Render {

	class TextureResourceManager : public ResourceManager< Texture> {
	public:
		Texture* loadImpl(const Name& id) override;
		void unloadImpl(Texture* texture);
		const Name& typeName()const override;
	public:
		void createNecessaryPersistenceResources()override;
	};


}

#endif