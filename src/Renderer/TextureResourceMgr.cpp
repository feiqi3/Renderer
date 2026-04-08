#include "Renderer/TextureResourceMgr.h"
#include "Common/ResourceSystem.h"
namespace Render{

	namespace {
		Texture* getErrorTexture() {
			ImageRaw* imageRaw = ImageRaw::createImageRaw(8, 8, 4, false);
			u8* imageData = (u8*)imageRaw->getImageRaw();

			int width = imageRaw->getWidth();
			int height = imageRaw->getHeight();

			int checkSize = 4;

			for (int i = 0; i < width; ++i) {
				for (int j = 0; j < height; ++j) {
					int idx = 4 * (i * height + j);

					bool isWhite = ((i / checkSize) + (j / checkSize)) % 2 == 0;

					u8 color = isWhite ? 255 : 0;

					imageData[idx + 0] = color; // R
					imageData[idx + 1] = color; // G
					imageData[idx + 2] = color; // B
					imageData[idx + 3] = 255;   // A 
				}
			}

			auto texture = imageRaw->toTextureResource();
			delete imageRaw;
			return texture;
		}
	}

	Texture* TextureResourceManager::loadImpl(const Name& id)
	{
		auto imageRaw = ImageRaw::createImageRaw(id.c_str(), -1);
		if (imageRaw == nullptr) {
			return nullptr;
		}
		Texture* ret = new Texture();
		ret->pImage = imageRaw->updateToGPU();
		ret->mState = ResourceLoadState::Loaded;
		delete imageRaw;
		return ret;
	}

	void TextureResourceManager::unloadImpl(Texture* texture)
	{
		RenderSystem::instance()->destroyImage(texture->getRsImage());
		delete texture;
	}

	const Name& TextureResourceManager::typeName() const
	{
		return Texture::typeName();
	}

	const Name& TextureResourceManager::getDefaultResourceName() const
	{
		return mDefaultResourceName;
	}

	Render::TexturePtr TextureResourceManager::getOrCreateCubemap(const Name& name)
	{

	}

	void TextureResourceManager::createNecessaryPersistenceResources() {
		mDefaultResourceName = Name("Builtin::ErrorRGB");
		this->registerResource(mDefaultResourceName, getErrorTexture(), ResourceLifetime::Persistent, nullptr);
	}

}

