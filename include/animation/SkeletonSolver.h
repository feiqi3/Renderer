#ifndef SKELETON_SOLVER_H_
#define SKELETON_SOLVER_H_
#pragma once
#include "Animation.h"
#include "Skeleton.h"

namespace Render::Anm {

	//Restore states to accelerate animation calculation
	struct SkeletonSolverState {
		void resetBySkeletonAndAnimation(const Skeleton* skeleton, const SkeletonAnimation* anm);

		struct JointState {
			int32_t lastTimeRotSearchId		= 0;
			int32_t lastTimeTransSearchId	= 0;
			int32_t lastTimeScaleSearchId	= 0;
		};
		std::vector<JointState> mJointStates;
		std::vector<uint32_t>   mJointToAnmIdx;

#if defined(DEBUG) || defined(_DEBUG)
		std::vector<uint8_t>	mIsJointUpdated;
#endif

	};

	//Static class
	class SkeletonSolver {
	public:
		static SkeletonSolverState*  createSkeletonSolverState(const Skeleton* skeleton, const SkeletonAnimation* anm);
		static void				     destroySkeletonSolverState(SkeletonSolverState* state);
		static void					 getAnimationPosAtTimeT(const Skeleton* skeleton, const SkeletonAnimation* anm,
			SkeletonState* sklState,SkeletonSolverState* solverState, float t, bool isBackSearch);

	};
}

#endif//SKELETON_SOLVER_H_