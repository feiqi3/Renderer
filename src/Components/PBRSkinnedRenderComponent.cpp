#include "components/PBRSkinnedRenderComponent.h"
#include "Renderer/SkeletonResourceManager.h"
#include "Renderer/AnimationResourceManager.h"
#include "Renderer/SkeletonResource.h"
#include "animation/SkeletonSolver.h"
#include "Renderer/DebugDrawManager.h"
#include "common/ResourceSystem.h"
#include "animation/Skeleton.h"
#include "function/Object.h"
namespace Render {
	void PBRSkinnedRenderComponent::onUpdate(float dt)
	{

		if (!mRenderSkeleton)return;
		if (this->mIsPlayAnm) {
			calculateSKinMatrix(dt);
			updateGPUSkinBuffer();
		}


	}

	void PBRSkinnedRenderComponent::setSkeleton(const SkeletonPtr& skl)
	{
		mRenderSkeleton = skl;
	}
	
	void PBRSkinnedRenderComponent::playBindingPos()
	{
		auto bindingPosPtr = ResourceSystem::instance()->getResource<SkeletonAnimationResource>(SkeletonAnimationResource::typeName(), Name("Builtin::DefaultBindingPosAnm"));
		this->playAnimation(bindingPosPtr, false);
	}


	void PBRSkinnedRenderComponent::playAnimation(const SkeletonAnimationPtr& animation, bool loop)
	{
		if (!animation) {
			playBindingPos();
			return;
		}
		mSkeletonAnimation = animation;
		this->mAnmPlayTime = 0.;
		this->mIsPlayAnm = true;
		this->mIsLoop = loop;
		this->mAnmLastSampleTime = -1;
	}

	void PBRSkinnedRenderComponent::updateGPUSkinBuffer()
	{
		if (mSavedSkinMatrics.size() > mJointsNum) {
			mJointsNum = mSavedSkinMatrics.size();
			rebuildGPUSkinBuffer(mJointsNum);
			static Name skinBufferName = Name("u_anmInfoList");
			for (auto& material : mMaterials) {
				material->bindParameter(skinBufferName, this->mSkeletonUpdateBuffer);
			}
		}

		RenderSystem::instance()->updateBufferData(
			mSkeletonUpdateBuffer, &mJointsNum, sizeof(uint32_t), 0
		);
		RenderSystem::instance()->updateBufferData(
			mSkeletonUpdateBuffer, mSavedSkinMatrics.data(), sizeof(mat4) * mJointsNum, 4 * sizeof(uint32_t)
		);
	}

	void PBRSkinnedRenderComponent::calculateSKinMatrix(float dt)
	{

		this->mSkeletonAnimation->sampleAnimationState(
			mRenderSkeleton, mAnimationState, mSolverState, mAnmPlayTime, false
		);
		//1. animation joint matrix
		mAnimationState->calculateModelSpaceMatrix(mRenderSkeleton->getSkeleton(), mSavedSkinMatrics);
		const auto& inverseBindingMatrics = mRenderSkeleton->getSkeleton()->mInverseBindingMats;
		//2. with Inverse binding matrix
		for (int i = 0;i < mRenderSkeleton->getSkeleton()->mInverseBindingMats.size();++i) {
			mSavedSkinMatrics[i] = mSavedSkinMatrics[i] * inverseBindingMatrics[i];
		}

		mAnmPlayTime += dt * mPlayRate;
		if (mAnmPlayTime >= mSkeletonAnimation->getSkeletonAnimation()->mDuration) {
			if (this->mIsLoop) {
				mAnmPlayTime = 0.;
			}
			else {
				mIsPlayAnm = false;
			}
		}

	}
	void PBRSkinnedRenderComponent::rebuildGPUSkinBuffer(uint32_t jointsNum)
	{
		RenderSystem::instance()->destroyBuffer(mSkeletonUpdateBuffer);
		mSkeletonUpdateBuffer = nullptr;
		uint32_t gpuJointBufferSize = sizeof(uint32_t) * 4 + jointsNum * sizeof(mat4);
		BufferDesc desc{};
		desc.bufUsage = BufferType_Storage;
		desc.byteSize = gpuJointBufferSize;
		desc.mappable = false;
		mSkeletonUpdateBuffer = RenderSystem::instance()->createBuffer(nullptr, gpuJointBufferSize, desc);
	}
}