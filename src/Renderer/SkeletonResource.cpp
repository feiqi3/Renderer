#include "SkeletonResource.h"
#include "renderer/SkeletonResource.h"
namespace Render {
	Anm::Skeleton* SkeletonResource::getSkeleton() const
	{
		return &mSkeleton;
	}

	const Render::Name& SkeletonResource::getTypeName() const
	{
		return typeName();
	}

	Render::ResourceMemory SkeletonResource::getMemory() const
	{
		//return sizeof skeleton
		ResourceMemory mem{};
		
		uint32_t cpuMem = 0;
		cpuMem += sizeof(Anm::Skeleton);
		cpuMem += sizeof(Name) * mSkeleton.mJointsName.size();
		cpuMem += sizeof(Joint) * mSkeleton.mJoints.size();
		cpuMem += sizeof(mat4) * mSkeleton.mJointsLocalTRS.size();
		cpuMem += sizeof(mat4) * mSkeleton.mInverseBindingMats.size();

		mem.cpuMemory = cpuMem;
		mem.gpuMemory = 0;

		return mem;
	}

	void SkeletonResource::setSkeleton(Anm::Skeleton&& rhs)
	{
		mSkeleton = std::move(rhs);
	}

	const Name& Render::SkeletonResource::typeName()
	{
		static const Name skeletonName = Name("Skeleton");
		return skeletonName;
	}
}
