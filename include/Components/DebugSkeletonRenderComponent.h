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
		virtual void onUpdate(float dt) override;
	private:
		SkeletonPtr mRenderSkeleton = nullptr;
		std::map<Name, SkeletonDrawConfig> mConfigs;
		Anm::SkeletonState* mBindingPosState = nullptr;
	};
}

#endif//SKELETON_RENDER_COMPONENT_H_