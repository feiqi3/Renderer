#include "animation/SkeletonSolver.h"
#include "render_log.h"

#include <map>

namespace Render::Anm {

    static inline void printAniAndSkeletonMismatchInfo(const Skeleton* skeleton, const SkeletonAnimation* anm) {
        if (!skeleton || !anm) return;
        std::string errString = "Animation {" + anm->mSkeletonAnimationName.str() + "} is mismatch with skeleton.";
        errString += "\nSkeleton has joints(" + std::to_string(skeleton->mJoints.size()) + "): ";
        for (int i = 0;i < skeleton->mJoints.size();++i) {
            const auto& name = skeleton->mJointsName[i];
            errString += "\t" + name.str();
        }
        errString += ".\nWhile animation has joints(" + std::to_string(anm->mJointAnimations.size()) + "): ";
        for (int i = 0;i < anm->mJointAnimations.size();++i) {
            const auto& name = anm->mJointAnimations[i].mJointName;
            errString += "\t" + name.str();
        }
        Log::error(errString);
        return;
    }

    void SkeletonSolverState::resetBySkeletonAndAnimation(const Skeleton* skeleton,const SkeletonAnimation* anm)
    {
        if (!skeleton || !anm) {
            mIsinit = false;
            return;
        }
        if (skeleton->mJoints.size() < anm->mJointAnimations.size()) {
            Log::error("Animation has more joints than skeleton, is that right?");
            printAniAndSkeletonMismatchInfo(skeleton, anm);
            mIsinit = false;
            return;
        }
        JointState defaultState{ 
            .lastTimeRotSearchId = -1,
            .lastTimeTransSearchId = -1,
            .lastTimeScaleSearchId = -1 
        };

        this->mJointStates.assign(skeleton->mJoints.size(), defaultState);
        this->mJointToAnmIdx.assign(skeleton->mJoints.size(), INT32_MAX);

        if (anm != mAnimation) {
            //use animation's joint to match skeleton's
            //cause in some case, animation may have less joint than skeleton 
            std::map<Name, uint32_t> skeletonJointsName;

            for (int i = 0;i < skeleton->mJointsName.size();++i) {
                const auto& jointAnm = skeleton->mJointsName[i];
                skeletonJointsName.insert({ jointAnm,i });
            }

            for (int i = 0;i < anm->mJointAnimations.size();++i) {
                const auto& jointName = anm->mJointAnimations[i].mJointName;
                auto itor = skeletonJointsName.find(jointName);
                if (itor == skeletonJointsName.end()) {
                    Log::error("Cannot find joint: {" + jointName.str() + "}");
                    printAniAndSkeletonMismatchInfo(skeleton, anm);
                    mIsinit = false;
                    return;
                }
                else {
                    //Map animation's joint index to skeleton 
                    mJointToAnmIdx[itor->second] = i;
                }
            }
        }

#if defined(DEBUG) || defined(_DEBUG)
        this->mIsJointUpdated.assign(skeleton->mJoints.size(), 0);
#endif //DEBUG || _DEBUG
        mSkeleton       = skeleton;
        mAnimation      = anm;
        mIsinit         = true;
    }

    
    SkeletonSolverState* Render::Anm::SkeletonSolver::createSkeletonSolverState(const Skeleton* skeleton, const SkeletonAnimation* anm)
    {
        SkeletonSolverState* solverState = new SkeletonSolverState();
        
        solverState->resetBySkeletonAndAnimation(skeleton, anm);

        return solverState;
    }

    void SkeletonSolver::destroySkeletonSolverState(SkeletonSolverState* state)
    {
        delete state;
    }

    void SkeletonSolver::getAnimationPosAtTimeT(const Skeleton* skeleton, const SkeletonAnimation* anm,
        SkeletonState* sklState, SkeletonSolverState* solverState, float t, bool isBackSearch)
    {
#if defined(DEBUG) || defined(_DEBUG)
        std::fill(solverState->mIsJointUpdated.begin(), solverState->mIsJointUpdated.end(), 0);
#endif //DEBUG || _DEBUG
        for (int i = 0;i < skeleton->mJoints.size();++i) {
            auto toAnmIdx = solverState->mJointToAnmIdx[i];
            Transform trans{};
            if (toAnmIdx != INT32_MAX) {
                const auto& anmJoint = anm->mJointAnimations[toAnmIdx];
                //Sample each track
                trans.rotation =
                    anmJoint.mRotTrack.sampleFromLast(t, solverState->mJointStates[i].lastTimeRotSearchId, isBackSearch);
                trans.scale =
                    anmJoint.mScaleTrack.sampleFromLast(t, solverState->mJointStates[i].lastTimeScaleSearchId, isBackSearch);
                trans.translation =
                    anmJoint.mTransTrack.sampleFromLast(t, solverState->mJointStates[i].lastTimeTransSearchId, isBackSearch);
            }
            else {
                //Use binding pos's instead
                trans = skeleton->mJoints[i];
            }
            sklState->mJointTransforms[i] = std::move(trans);
            //The space where joint is origin
            auto jointSpaceMat            = sklState->mJointTransforms[i].toMatrix();
            int32_t parIdx                = skeleton->mParents[i];
#if defined(DEBUG) || defined(_DEBUG)
            //Check is parent updated? or error may happen
            bool isParentUpdated = false;
            if (parIdx < 0) {
                isParentUpdated = true;//This is root
            }
            else {
                isParentUpdated = (solverState->mIsJointUpdated[parIdx] != 0);
            }
            if (!isParentUpdated) {
                Log::error("Animation error: updating child node before parent!!!!Check Skeleton!!!");
            }
            solverState->mIsJointUpdated[i] = 1;
#endif //DEBUG || _DEBUG
            if (parIdx >= 0) {
                const auto& parLocalMat     = sklState->mLocalMatrices[parIdx];
                sklState->mLocalMatrices[i] = parLocalMat * jointSpaceMat;
            }
            else {
                sklState->mLocalMatrices[i] = std::move(jointSpaceMat);
            }
        }

    }

}