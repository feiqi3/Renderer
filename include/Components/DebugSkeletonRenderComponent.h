#ifndef SKELETON_RENDER_COMPONENT_H_
#define SKELETON_RENDER_COMPONENT_H_

#include "function/Component.h"
#include "Renderer/GltfLoader.h"
#include "Renderer/MaterialInstance.h"
#include "Renderer/Mesh.h"
#include "Components/RenderComponent.h"
#include "Renderer/ResourceFwd.h"
namespace Render {
	class MaterialTemplate;
	class RenderEntity;
	namespace Anm {
		class SkeletonState;
		class SkeletonSolverState;
	}
	class SkeletonRenderComponent : public Component {
	public:
		struct SkeletonDrawConfig {
			float	BonelineWidth	= 0.005f;
			float	scaleJoint		= 1.f;
			vec4	colorBone		= vec4(0.2,0.9,0.3,1.);
			vec4	colorJoint		= vec4(1., 1., 1., 1.);
		};
		void setSkeletonJointDrawConfig(const Name& name, const SkeletonDrawConfig& cfg);
		void setSkeleton(const SkeletonPtr& inSkeleton);
		void playAnimation(const SkeletonAnimationPtr& inAnm, bool loop);
		void playBindingPos();
		void setPlayRate(float rate);
		void setJointScale(float scale);
		void setBoneWidth(float width);
		virtual void onUpdate(float dt) override;
	protected:
		void resetSkeletonState();
	private:
		SkeletonPtr mRenderSkeleton = nullptr;
		SkeletonAnimationPtr mSkeletonAnimation = nullptr;
		std::map<Name, SkeletonDrawConfig> mConfigs;
		Anm::SkeletonState* mBindingPosState = nullptr;
		bool mIsPlayAnm = false;
		bool mIsLoop = false;
		float mAnmPlayTime = 0.f;
		float mAnmLastSampleTime = 0.f;
		float mDuration = 0.f;
		float mPlayRate = 1.f;
		float mJointScale = 1.f;
		float mBoneWidth = 0.01f;
		Anm::SkeletonSolverState* mSolverState = nullptr;
	};
}

#endif//SKELETON_RENDER_COMPONENT_H_