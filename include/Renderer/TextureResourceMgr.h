#ifndef TEXTURE_RESOURCE_MANAGER_H_
#define TEXTURE_RESOURCE_MANAGER_H_
#include "Texture.h"

namespace Render {
	template<>
	inline Texture* ResourceManager<Texture>::loadImpl(const Name& id) {
		auto imageRaw = ImageRaw::createImageRaw(id.c_str(), -1);
		Texture* ret = new Texture();
		ret->pImage = imageRaw->updateToGPU();
		auto& memory = ret->mMemory;
		memory.cpuMemory = sizeof(Texture) + sizeof(rs_image);
		memory.gpuMemory = imageRaw->getByteSize();
		delete imageRaw;
		return ret;
	}

	template<>
	inline const Name& ResourceManager<Texture>::typeName()const { const static auto textureName = Name("Texture"); return textureName; }

	template<>
	inline void ResourceManager<Texture>::unloadImpl(Texture* texture) {
		RenderSystem::instance()->destroyImage(texture->getRsImage());
	}
}

#endif