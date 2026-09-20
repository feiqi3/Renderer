#ifndef RESOURCE_FWD_H
#define RESOURCE_FWD_H
#include "common/ResourceHandler.h"
namespace Render {
	class Texture;
	class Material;
	class RenderPass;
	class RenderEntity;
	class SkeletonResource;
	class SkeletonAnimationResource;
	using TexturePtr	= ResourceHandle<Texture>;
	using MaterialPtr	= ResourceHandle<Material>;
	using SkeletonPtr   = ResourceHandle<SkeletonResource>;
	using SkeletonAnimationPtr = ResourceHandle<SkeletonAnimationResource>;
}
#endif