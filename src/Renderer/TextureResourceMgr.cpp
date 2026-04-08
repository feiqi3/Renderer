#include "Renderer/TextureResourceMgr.h"
#include "Common/ResourceSystem.h"
#include "platform/FileSystem/FileSystem.h"
namespace Render{

	static std::vector<std::string> nameOfCubeMapFace = { "px","nx","py","ny","pz","nz" };

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
		//1. open folder   
		Platform::FileSystem* fs = Platform::FileSystem::instance();
		auto listFilesOfDirectory = fs->listDirectory(name.str());
		std::vector<std::string> listOfFoundFace;
		listOfFoundFace.reserve(6);
		for (const auto& name : nameOfCubeMapFace) {
			for (const auto& fileName : listFilesOfDirectory) {
				bool found = false;
				if (fileName.starts_with(fileName)) {
					listOfFoundFace.push_back(fileName);
					found = true;
					break;
				}
				if (!found)
				{
					assert(0 && "Cubemap face not found");
					return nullptr;
				}
			}
		}
		int x = -1,y = -1,channel = -1;

		bool isHdr = false;
		std::vector<ImageRaw*> images;
		ImageFormat format;
		for (const auto& fileNames : listOfFoundFace) {
			auto imageRaw = ImageRaw::createImageRaw((name.str() +"/" + fileNames).c_str(), -1);
			if (x < 0 || y < 0 || channel < 0) {
				x = imageRaw->getWidth();
				y = imageRaw->getHeight();
				channel = imageRaw->getChannel();
				isHdr = imageRaw->isHdr();
				format = imageRaw->getFormat();
			}
			else {
				bool isImageAttributeSame = (imageRaw->getWidth() == x && imageRaw->getHeight() == y && imageRaw->getChannel() == channel && "Cubemap face size or channel mismatch");
				if (!isImageAttributeSame) {
					for (auto img : images) {
						delete img;
					}
					return nullptr;
				}
			}

			if (isHdr != imageRaw->isHdr()) {
				assert(0 && "Cubemap face HDR mismatch");
				for (auto img : images) {
					delete img;
				}
				return nullptr;
			}

			images.push_back(imageRaw);
		}

		//2. create cubemap texture
		auto image = RenderSystem::instance()->createCubemap(nullptr, 0, format, x, y, 1, 1, 1);
		for (int i = 0; i < 6; ++i) {
			RenderSystem::instance()->updateImageData(
				image,
				images[i]->getImageRaw(), images[i]->getByteSize(),
				0, 0, 0, x, y, 1,i, 1, 1
			);
			delete images[i];
		}
		Texture* tex = new Texture();
		tex->pImage = image;
		tex->mState = ResourceLoadState::Loaded;
		auto entry = this->registerResource(name, tex, ResourceLifetime::Transient, nullptr);
		return ResourceHandle<Texture>(this,entry);
	}

	void TextureResourceManager::createNecessaryPersistenceResources() {
		mDefaultResourceName = Name("Builtin::ErrorRGB");
		this->registerResource(mDefaultResourceName, getErrorTexture(), ResourceLifetime::Persistent, nullptr);
	}

}

