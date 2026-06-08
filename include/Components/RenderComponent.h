#ifndef RENDER_COMPONENT_H_
#define RENDER_COMPONENT_H_

#include "function/Component.h"
#include "function/Scene.h"
#include <vector>

namespace Render {
	class RenderEntity;

	class RenderComponent : public Component {
	public:
		RenderComponent();
		virtual ~RenderComponent() = default;

		virtual void collectRenderEntities(std::vector<RenderEntity*>& outEntities) = 0;

		uint32_t getCullMask() const { return mCullMask; }
		void setCullMask(uint32_t layer) { mCullMask = layer; }

		bool isCastShadow() const { return mCastShadow; }
		void setCastShadow(bool cast) { mCastShadow = cast; }

		size_t getSceneIndex() const { return mSceneIndex; }
		void setSceneIndex(size_t idx) { mSceneIndex = idx; }

		void onOwnerSetScene(Scene* originScene, Scene* scene) override;

	protected:
		uint32_t mCullMask;
		bool mCastShadow;
		size_t mSceneIndex;
	};
}

#endif