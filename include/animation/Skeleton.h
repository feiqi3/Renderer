#if !defined(SKELETON_H_)
#define SKELETON_H_
#pragma once

#include "common/CommonMath.h"
#include "common/Name.h"
#include "animation/animation.h"
#include <vector>
namespace Render::Anm {


	using Joint = Transform;
	
	class Skeleton {
	public:
		std::vector<Name>		mJointsName;
		std::vector<Joint>		mJoints;
		std::vector<mat4>		mJointsLocalTRS;
		std::vector<mat4>		mInverseBindingMats;
		std::vector<int32_t>	mParents;
	};

	class SkeletonState {
	public:
		void resize(size_t jointCount);

		std::vector<Transform> mJointTransforms;
		std::vector<mat4>	   mLocalMatrices;
	};

}
#endif//!SKELETON_H_