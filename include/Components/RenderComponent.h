#ifndef RENDER_COMPONENT_H_
#define RENDER_COMPONENT_H_

#include "function/Component.h"
#include "function/Scene.h"
#include <vector>

namespace Render {
	class RenderEntity;

	class RenderComponent : public Component {
	public:
		RenderComponent()
			: mLayer(1 << 0)
			, mCastShadow(true)
			, mSceneIndex(static_cast<size_t>(-1)) 
		{
		}
		virtual ~RenderComponent() = default;

		virtual const std::vector<RenderEntity*>& getRenderEntities() const = 0;

		uint32_t getLayer() const { return mLayer; }
		void setLayer(uint32_t layer) { mLayer = layer; }

		bool isCastShadow() const { return mCastShadow; }
		void setCastShadow(bool cast) { mCastShadow = cast; }

		size_t getSceneIndex() const { return mSceneIndex; }
		void setSceneIndex(size_t idx) { mSceneIndex = idx; }

		void onOwnerSetScene(Scene* originScene, Scene* scene) override;

	protected:
		uint32_t mLayer;
		bool mCastShadow;
		size_t mSceneIndex;
	};
}

#endif