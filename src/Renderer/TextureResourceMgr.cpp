#include "Renderer/TextureResourceMgr.h"

namespace Render{

	namespace {
		Texture* getErrorTexture() {
			ImageRaw* imageRaw = ImageRaw::createImageRaw(8, 8, 4, false);
			u8* imageData = (u8*)imageRaw->getImageRaw();
			for (int i = 0; i < imageRaw->getWidth(); ++i) {
				for (int j = 0; j < imageRaw->getHeight(); ++j) {
					int idx = 4 * (i * imageRaw->getHeight() + j);
					imageData[idx + 0] = 238;
					imageData[idx + 1] = 130;
					imageData[idx + 2] = 238;
					imageData[idx + 3] = 255;
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
		ret->mState = ResourceState::Loaded;
		delete imageRaw;
		return ret;
	}

	void TextureResourceManager::unloadImpl(Texture* texture)
	{
		RenderSystem::instance()->destroyImage(texture->getRsImage());
	}

	const Name& TextureResourceManager::typeName() const
	{
		return Texture::typeName();
	}

	void TextureResourceManager::createNecessaryPersistenceResources() {
		this->registerResource(Name("Builtin::ErrorRGB"), getErrorTexture(), ResourceLifetime::Persistent, nullptr);
	}

}

