#ifndef ANIMATION_RESOURCE_H_
#define ANIMATION_RESOURCE_H_
#include "animation/Animation.h"
#include "common/ResourceManager.h"
#include "renderer/ResourceFwd.h"
#include <map>
namespace Render {
	namespace Anm {
		class SkeletonAnimation;
		class SkeletonState;
		class SkeletonSolverState;
	};

	class SkeletonAnimationResource : public IResource {
	public:
		static const Name& typeName();
		virtual const Name& getTypeName() const override;
		float getSampleRate()	const;
		float getDuration()		const;

		const Anm::SkeletonAnimation* getSkeletonAnimation()const;
		void setSkeletonAnimation(Anm::SkeletonAnimation&& inAnm);
		//solverState -> a structure to restore the state that can accelerate animation sample
		//skeletonState -> restore final animation state
		//reverse -> is a reverse play? This can affect sample efficiency
		void sampleAnimationState(SkeletonPtr skeleton, Anm::SkeletonState*& skeletonState,Anm::SkeletonSolverState*& solverState,float t, bool reverse);
		virtual ResourceMemory getMemory() const override;

	private:
		Anm::SkeletonAnimation mAnimation;
	};
}
#endif//ANIMATION_RESOURCE_H_