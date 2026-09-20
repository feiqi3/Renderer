#include "renderer/AnimationResource.h"
#include "renderer/SkeletonResource.h"
#include "animation/animation.h"
#include "animation/skeleton.h"
#include "animation/skeletonsolver.h"
namespace Render {
	const Name& SkeletonAnimationResource::typeName()
	{
		const static Name name = Name("Animation");
		return name;
	}

	const Name& SkeletonAnimationResource::getTypeName() const
	{
		return getTypeName();
	}

	float SkeletonAnimationResource::getSampleRate() const
	{
		return mAnimation.mSampleRate;
	}

	float SkeletonAnimationResource::getDuration() const
	{
		return mAnimation.mDuration;
	}

	const Anm::SkeletonAnimation* SkeletonAnimationResource::getSkeletonAnimation() const
	{
		return &mAnimation;
	}
	void SkeletonAnimationResource::setSkeletonAnimation(Anm::SkeletonAnimation&& inAnm) 
	{
		mAnimation = std::move(inAnm);
	}

	void SkeletonAnimationResource::sampleAnimationState(SkeletonPtr skeleton, Anm::SkeletonState*& skeletonState, Anm::SkeletonSolverState*& solverState, float t, bool isBack)
	{
		if (!skeleton) {
			return;
		}

		if (skeletonState == nullptr) {
			skeletonState = new Anm::SkeletonState();
		}

		if (!solverState) {
			solverState = new Anm::SkeletonSolverState();
		}
		Anm::SkeletonSolver::createSkeletonSolverState(skeleton->getSkeleton(), this->getSkeletonAnimation());
	
		if (solverState->mIsinit == false)return;
		Anm::SkeletonSolver::getAnimationPosAtTimeT(
			skeleton->getSkeleton(), this->getSkeletonAnimation(), skeletonState, solverState, t,isBack
		);

	}
	ResourceMemory SkeletonAnimationResource::getMemory() const
	{
		ResourceMemory mem{};
		mem.cpuMemory += sizeof(this);
		mem.cpuMemory += sizeof(Anm::SkeletonAnimation);
		mem.cpuMemory += sizeof(Anm::JointAnimation) * mAnimation.mJointAnimations.capacity();
		for (const auto& jointAnm : mAnimation.mJointAnimations) {
			mem.cpuMemory += jointAnm.mRotTrack.mKeyData.capacity() * sizeof(Anm::RotationKey);
			mem.cpuMemory += jointAnm.mTransTrack.mKeyData.capacity() * sizeof(Anm::TranslationKey);
			mem.cpuMemory += jointAnm.mScaleTrack.mKeyData.capacity() * sizeof(Anm::ScaleKey);
		}
		return mem;
	}
}