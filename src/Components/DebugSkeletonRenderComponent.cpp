#include "Components/DebugSkeletonRenderComponent.h"
#include "Renderer/SkeletonResourceManager.h"
#include "Renderer/AnimationResourceManager.h"
#include "Renderer/SkeletonResource.h"
#include "animation/SkeletonSolver.h"
#include "Renderer/DebugDrawManager.h"
#include "common/ResourceSystem.h"
#include "animation/Skeleton.h"
#include "function/Object.h"
namespace Render {
	void SkeletonRenderComponent::setSkeletonJointDrawConfig(const Name& name, const SkeletonDrawConfig& cfg)
	{
		this->mConfigs[name] = cfg;
	}
	void SkeletonRenderComponent::setSkeleton(const SkeletonPtr& inSkeleton)
	{
		mRenderSkeleton = inSkeleton;
		if (inSkeleton == nullptr) {
			delete mAnimationState;
			mAnimationState = nullptr;
		}
		if (mSkeletonAnimation == nullptr) {
			playBindingPos();
		}
	}
	void SkeletonRenderComponent::playAnimation(const SkeletonAnimationPtr& inAnm,bool loop)
	{
		this->mAnmPlayTime = 0.;
		this->mIsPlayAnm = true;
		this->mIsLoop = loop;
		this->mAnmLastSampleTime = -1;
		if (inAnm == nullptr) {
			playBindingPos();
			return;
		}

		if (mSkeletonAnimation != inAnm) {
			this->mSkeletonAnimation = inAnm;
			resetSkeletonState();
		}
	}

	void SkeletonRenderComponent::playBindingPos()
	{
		if (mRenderSkeleton == nullptr)return;
		auto bindingPosPtr = ResourceSystem::instance()->getResource<SkeletonAnimationResource>(SkeletonAnimationResource::typeName(), Name("Builtin::DefaultBindingPosAnm"));
		this->playAnimation(bindingPosPtr, false);
	}

	void SkeletonRenderComponent::setPlayRate(float rate)
	{
		mPlayRate = rate;
	}

	void SkeletonRenderComponent::setJointScale(float scale)
	{
		mJointScale = scale;
	}

	void SkeletonRenderComponent::setBoneWidth(float width)
	{
		mBoneWidth = width;
	}

	void SkeletonRenderComponent::onUpdate(float dt)
	{
		if (!mRenderSkeleton)return;
		if (this->mIsPlayAnm) {
			this->mSkeletonAnimation->sampleAnimationState(
				mRenderSkeleton, mAnimationState, mSolverState, mAnmPlayTime, false
			);
			mAnimationState->calculateModelSpaceMatrix(mRenderSkeleton->getSkeleton(), mSavedPosModelSpaceMat);
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
		std::vector<vec3> loc;
		
		mat4 localMat(1.);
		if (this->owner()) {
			auto obj = this->owner();
			localMat = getTRS(
				obj->localPosition(),obj->localRotation(), obj->localScale()
			);
		}

		const auto& skl = mRenderSkeleton->getSkeleton();
		for (int i = 0;i < mSavedPosModelSpaceMat.size();++i) {
			auto transformMat = localMat * mSavedPosModelSpaceMat[i];
			vec3 trans;
			vec3 scale;
			quat rot;
			vec3 unused0;
			vec4 unused1;
			float scaleLocal = std::min(scale.x, std::min(scale.y, scale.z));
			decomposeTRS(transformMat, scale, rot, trans, unused0, unused1);
			loc.push_back(trans);
			auto itor = mConfigs.find(skl->mJointsName[i]);
			if (itor == mConfigs.end()) {
				DebugDrawManager::instance()->drawCube(trans, rot, vec4(1., 1., 1., 1), scaleLocal * mJointScale * 1.f);
			}
			else {
				const auto& config = itor->second;
				DebugDrawManager::instance()->drawCube(trans, rot, config.colorJoint, mJointScale * config.scaleJoint);
			}
		}
		const auto& skeletonPar = mRenderSkeleton->getSkeleton()->mParents;
		for (int i = 0; i < skeletonPar.size();++i) {
			auto skeletonParId = skeletonPar[i];
			if (skeletonPar[i] < 0) {
				continue;
			}
			vec3 beg = loc[skeletonParId];
			vec3 end = loc[i];
			auto itor = mConfigs.find(skl->mJointsName[skeletonParId]);
			if (itor == mConfigs.end()) {
				DebugDrawManager::instance()->drawLine(beg, end, vec4(0, 1., 0., 1.), mBoneWidth);
			}
			else {
				const auto& config = itor->second;
				DebugDrawManager::instance()->drawLine(beg, end, config.colorBone, mBoneWidth * config.BonelineWidth);
			}
		}

	}
	void SkeletonRenderComponent::resetSkeletonState()
	{
		if (this->mSolverState != nullptr && mSkeletonAnimation != nullptr && mRenderSkeleton != nullptr) {
			this->mSolverState->resetBySkeletonAndAnimation(mRenderSkeleton->getSkeleton(), mSkeletonAnimation->getSkeletonAnimation());
		}
	}
}
