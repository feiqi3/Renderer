#include "Renderer/SkeletonResourceManager.h"
#include <exception>
namespace Render {
	
	const Render::Name& SkeletonResourceManager::typeName() const
	{
		return SkeletonResource::typeName();
	}

	Render::SkeletonPtr SkeletonResourceManager::createSkeletonResource(Anm::Skeleton&& inSkeleton)
	{
		auto skeletonRes = new SkeletonResource;
		skeletonRes->setSkeleton(std::move(inSkeleton));
		auto entry = this->registerAnonymousResource(skeletonRes, ResourceLifetime::Transient, nullptr);
		return SkeletonPtr(this, entry);
	}

	Render::SkeletonPtr SkeletonResourceManager::createSkeletonResource(const Name& name, Anm::Skeleton&& inSkeleton)
	{
		auto skeletonRes = new SkeletonResource;
		skeletonRes->setSkeleton(std::move(inSkeleton));
		auto entry = this->registerResource(name, skeletonRes, ResourceLifetime::Transient, nullptr);
		return SkeletonPtr(this, entry);
	}

	Render::SkeletonResource* SkeletonResourceManager::loadImpl(const Name& id)
	{
		throw std::runtime_error("SamplerResourceManager::loadImpl should not be called directly.");
		return nullptr;
	}

	void SkeletonResourceManager::unloadImpl(SkeletonResource* skeleton)
	{
		delete skeleton;
	}

}