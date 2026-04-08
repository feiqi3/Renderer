#ifndef TEXTURE_RESOURCE_MANAGER_H_
#define TEXTURE_RESOURCE_MANAGER_H_
#include "Texture.h"
#include "common/ResourceManager.h"
#include "common/Singleton.h"
namespace Render {

	class TextureResourceManager : public ResourceManager< Texture>,public Singleton<TextureResourceManager> {
	public:
		Texture* loadImpl(const Name& id) override;
		void unloadImpl(Texture* texture);
		const Name& typeName()const override;
		const Name& getDefaultResourceName()const override;

		//in: folder name
		//there should be 6 textures in the folder with name: right, left, top, bottom, front, back.
		TexturePtr getOrCreateCubemap(const Name& name);

	public:
		void createNecessaryPersistenceResources()override;
		Name mDefaultResourceName;
	};


}

#endif