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
		std::vector<mat4>		mInverseBindingMats;
		std::vector<int32_t>	mParents;
	};

	//Class restore
	struct SkeletonState {
	public:
		void resize(size_t jointCount);
		//Cause we dont record parent things inside state, so we need extra info from skeleton
		bool calculateModelSpaceMatrix(const Skeleton* inSkeleton,std::vector<mat4>& outLocalMatrics);
		std::vector<Transform> mJointTransforms;
	};

}
#endif//!SKELETON_H_