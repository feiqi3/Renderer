#include "Components/DebugSkeletonRenderComponent.h"
#include "Renderer/SkeletonResourceManager.h"
#include "Renderer/AnimationResourceManager.h"
#include "Renderer/SkeletonResource.h"
#include "Renderer/DebugDrawManager.h"
#include "common/ResourceSystem.h"
#include "animation/Skeleton.h"
namespace Render {
	void SkeletonRenderComponent::setSkeletonJointDrawConfig(const Name& name, const SkeletonDrawConfig& cfg)
	{
		this->mConfigs[name] = cfg;
	}
	void SkeletonRenderComponent::setSkeleton(const SkeletonPtr& inSkeleton)
	{
		mRenderSkeleton = inSkeleton;
		if (inSkeleton == nullptr) {
			delete mBindingPosState;
			mBindingPosState = nullptr;
		}
		//Get default binding pos state
		auto bindingPosPtr = ResourceSystem::instance()->getResource<SkeletonAnimationResource>(SkeletonAnimationResource::typeName(), Name("Builtin::DefaultBindingPosAnm"));
		Anm::SkeletonSolverState* solverState = nullptr;
		bindingPosPtr->sampleAnimationState(mRenderSkeleton, mBindingPosState, solverState, 0, false);
		delete solverState;
	}
	void SkeletonRenderComponent::onUpdate(float dt)
	{
		if (!mRenderSkeleton)return;
		std::vector<vec3> loc;
		const auto& skl = mRenderSkeleton->getSkeleton();
		for (int i = 0;i < mBindingPosState->mJointTransforms.size();++i) {
			const auto& transformMat = mBindingPosState->mLocalMatrices[i];
			vec3 trans;
			vec3 scale;
			quat rot;
			vec3 unused0;
			vec4 unused1;
			decomposeTRS(transformMat, scale, rot, trans, unused0, unused1);
			loc.push_back(trans);
			auto itor = mConfigs.find(skl->mJointsName[i]);
			if (itor == mConfigs.end()) {
				DebugDrawManager::instance()->drawCube(trans, rot, vec4(1., 1., 1., 1), .5f);
			}
			else {
				const auto& config = itor->second;
				DebugDrawManager::instance()->drawCube(trans, rot, config.colorJoint, 0.01f * config.scaleJoint);
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
				DebugDrawManager::instance()->drawLine(beg, end, vec4(0, 1., 0., 1.), 0.005f);
			}
			else {
				const auto& config = itor->second;
				DebugDrawManager::instance()->drawLine(beg, end, config.colorBone, config.BonelineWidth);
			}
		}

	}
}
