#ifndef ANIMATION_RESOURCE_MANAGER_H_
#define ANIMATION_RESOURCE_MANAGER_H_
#include "Renderer/AnimationResource.h"
namespace Render {
	
	class SkeletonAnimationResourceManager : public ResourceManager< SkeletonAnimationResource>, public Singleton<SkeletonAnimationResourceManager> {

		const Name& typeName()const override;
		SkeletonAnimationPtr createSkeletonAnimationResource(Anm::SkeletonAnimation&& inAnm);
		SkeletonAnimationPtr createSkeletonAnimationResource(const Name& name, Anm::SkeletonAnimation&& inAnm);
		SkeletonAnimationResource* loadImpl(const Name& id) override;
		void unloadImpl(SkeletonResource* skeleton);
	};
}

#endif//ANIMATION_RESOURCE_MANAGER_H_