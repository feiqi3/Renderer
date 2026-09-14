#include "animation/Skeleton.h"

namespace Render::Anm {

	void SkeletonState::resize(size_t jointCount)
	{
		mJointTransforms.resize(jointCount);
		mLocalMatrices.resize(jointCount);
	}

}
