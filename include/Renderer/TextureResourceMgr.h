#ifndef TEXTURE_RESOURCE_MANAGER_H_
#define TEXTURE_RESOURCE_MANAGER_H_
#include "Texture.h"
#include "render_resource_def.h"
#include "common/ResourceManager.h"
#include "common/Singleton.h"
namespace Render {

	class TextureResourceManager : public ResourceManager< Texture>,public Singleton<TextureResourceManager> {
	public:
		TexturePtr createEmpty(const Name& id);
		//Create a anonymous resource
		TexturePtr createEmpty();
		TexturePtr createRenderTexture(RenderTextureFormat format, uint32_t width, uint32_t height, uint32_t depth,uint32_t mips, uint32_t arrayLayers, bool UAV);
		Texture* loadImpl(const Name& id) override;
		void unloadImpl(Texture* texture);
		const Name& typeName()const override;
		const Name& getDefaultResourceName()const override;
		TexturePtr createFromRsImage(const Name& name,rs_image* img);
		TexturePtr getDefaultTexture();
		//in: folder name
		//there should be 6 textures in the folder with name: right, left, top, bottom, front, back.
		TexturePtr getOrCreateCubemap(const Name& name,int mip = -1);

	public:
		void createNecessaryPersistenceResources()override;
		Name mDefaultResourceName;
	
	private:
		TexturePtr mDefaultTexture;

	};


}

#endif