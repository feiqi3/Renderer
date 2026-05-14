#ifndef RESOURCE_FWD_H
#define RESOURCE_FWD_H
#include "common/ResourceHandler.h"
namespace Render {
	class Texture;
	class Material;
	class MaterialInstance;
	class RenderPass;
	class RenderEntity;

	using TexturePtr	= ResourceHandle<Texture>;
	using MaterialPtr	= ResourceHandle<Material>;

}
#endif