#ifndef PBR_SKINNED_RENDER_COMPONENT_H_
#define PBR_SKINNED_RENDER_COMPONENT_H_
#include "components/PBRRenderComponent.h"
#include "Renderer/ResourceFwd.h"
namespace Render {
	namespace Anm {
		struct SkeletonState;
		struct SkeletonSolverState;
	}

	class PBRSkinnedRenderComponent : public PBRRenderComponent {
	public:

		void playBindingPos();
		void onUpdate(float dt);
		void setSkeleton(const SkeletonPtr& skl);
		void playAnimation(const SkeletonAnimationPtr& animation, bool loop);

	protected:
		void updateGPUSkinBuffer();
		void calculateSKinMatrix(float dt);
		void rebuildGPUSkinBuffer(uint32_t jointsNum);
	private:
		std::vector<mat4> mSavedSkinMatrics;

		SkeletonPtr mRenderSkeleton = nullptr;
		SkeletonAnimationPtr mSkeletonAnimation = nullptr;

		Anm::SkeletonSolverState* mSolverState = nullptr;
		Anm::SkeletonState* mAnimationState = nullptr;


		bool mIsPlayAnm = false;
		bool mIsLoop = false;
		float mAnmPlayTime = 0.f;
		float mAnmLastSampleTime = 0.f;
		float mDuration = 0.f;
		float mPlayRate = 1.f;
		rs_buffer* mSkeletonUpdateBuffer = nullptr;
		uint32_t mJointsNum = 0;
	};

}
#endif//PBR_SKINNED_RENDER_COMPONENT_H_