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

		inline uint32_t getCullMask() const { return mCullMask; }
		inline void setCullMask(uint32_t layer) { mCullMask = layer; }

		inline bool isCastShadow() const { return mCastShadow; }
		inline void setCastShadow(bool cast) { 
			bool triggerChange = mCastShadow != cast;
			mCastShadow = cast; 
			if (triggerChange) {
				onShadowSettingChanged();
			}
		}

		inline virtual void onShadowSettingChanged() {};

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