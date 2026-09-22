#include "animation/Skeleton.h"
#include "render_log.h"
namespace Render::Anm {

	void SkeletonState::resize(size_t jointCount)
	{
		mJointTransforms.resize(jointCount);
	}

	bool SkeletonState::calculateModelSpaceMatrix(const Skeleton* inSkeleton, std::vector<mat4>& outModelMatrics)
	{
		if (inSkeleton->mJoints.size() != this->mJointTransforms.size()) {
			Log::error("SkeletonState seems tobe mismatch with this skeleton.");
			return false;
		}

		outModelMatrics.resize(inSkeleton->mJoints.size(),mat4(1.));

		for (int i = 0;i < inSkeleton->mJoints.size();++i) {
			int parIdx = inSkeleton->mParents[i];
			if (parIdx < 0) {
				outModelMatrics[i] = mJointTransforms[i].toMatrix();
			}
			else {
				//				 ParJoint in model space mat * localSpace where joint is origin
				outModelMatrics[i] = outModelMatrics[parIdx] * mJointTransforms[i].toMatrix();
			}
		}
		return true;

	}

}
