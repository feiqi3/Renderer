#if !defined(SKELETON_RESOURCE_MANAGER_H_)
#define SKELETON_RESOURCE_MANAGER_H_
#include "renderer/SkeletonResource.h"
#include "common/Singleton.h"
#include "common/ResourceManager.h"
#include "Renderer/ResourceFwd.h"

namespace Render {
	class SkeletonResourceManager : public ResourceManager< SkeletonResource>, public Singleton<SkeletonResourceManager> {
	
		const Name& typeName()const override;
		SkeletonPtr createSkeletonResource(Anm::Skeleton&& inSkeleton);
		SkeletonPtr createSkeletonResource(const Name& name,Anm::Skeleton&& inSkeleton);
		SkeletonResource* loadImpl(const Name& id) override;
		void unloadImpl(SkeletonResource* skeleton);
	};

}



#endif //!SKELETON_RESOURCE_MANAGER_H_