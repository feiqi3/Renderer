#include "Renderer/AnimationResourceManager.h"
#include <exception>
namespace Render {

	const Name& Render::SkeletonAnimationResourceManager::typeName() const
	{
		return SkeletonAnimationResource::typeName();
	}

	Render::SkeletonAnimationPtr 
		SkeletonAnimationResourceManager::createSkeletonAnimationResource(Anm::SkeletonAnimation&& inSkeleton)
	{
		auto skeletonAnmRes = new SkeletonAnimationResource();
		skeletonAnmRes->setSkeletonAnimation(std::move(inSkeleton));
		auto entry = this->registerAnonymousResource(skeletonAnmRes, ResourceLifetime::Transient, nullptr);
		return SkeletonAnimationPtr(this, entry);
	}

	Render::SkeletonAnimationPtr SkeletonAnimationResourceManager::createSkeletonAnimationResource(const Name& name, Anm::SkeletonAnimation&& inSkeleton)
	{
		auto skeletonRes = new SkeletonAnimationResource;
		skeletonRes->setSkeletonAnimation(std::move(inSkeleton));
		auto entry = this->registerResource(name, skeletonRes, ResourceLifetime::Transient, nullptr);
		return SkeletonAnimationPtr(this, entry);
	}

	SkeletonAnimationResource* SkeletonAnimationResourceManager::loadImpl(const Name& id)
	{
		throw std::runtime_error("SkeletonAnimationResourceManager::loadImpl should not be called directly.");
		return nullptr;
	}

	void SkeletonAnimationResourceManager::unloadImpl(SkeletonAnimationResource* skeleton)
	{
		delete skeleton;
	}

	void SkeletonAnimationResourceManager::createNecessaryPersistenceResources()
	{
		Anm::SkeletonAnimation defaultBindingPosAnm{};
		auto defaultBindingPosAnmName = Name("Builtin::DefaultBindingPosAnm");
		defaultBindingPosAnm.mSkeletonAnimationName = defaultBindingPosAnmName;
		mDefaultPosPtr = this->createSkeletonAnimationResource(
			defaultBindingPosAnmName,std::move(defaultBindingPosAnm)
		);
	}

}
