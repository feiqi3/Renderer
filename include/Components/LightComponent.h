#ifndef LIGHT_COMPONENT_H_
#define LIGHT_COMPONENT_H_

#include "Renderer/Light.h"
#include "function/Component.h"
#include "Components/RenderComponent.h"
#include "common/CommonMath.h"
#include <memory>
namespace Render {
	class PBRRenderComponent;
	class LightComponent : public  Component {
	public:
		LightComponent(LightType type);
		inline void setHasShadow(bool hasShadow) {
			mLight->setHasShadow(hasShadow);
		}
		inline bool getHasShadow()const { return mLight->getHasShadow(); }
		Light* getLight();
		void onEnable() override;
		void onDisable() override;
		void onTransformChanged() override;
		void setColor(vec3 color);
		void setEnableLightDelegate(bool d);
		virtual void onLightDelegateEnableChanged();
		void setIntensity(float i);
		void onUpdate(float dt) override {}
	protected:
		bool mLightDelegate = true;
		std::unique_ptr<Light> mLight = nullptr;
		int mLightIdx = -1;
	};

	class PointLightComponent : public  LightComponent {
	public:
		PointLightComponent();
		void onLightDelegateEnableChanged() override;
		void onDisable() override;
		void onEnable() override;
		void setRange(float r);
		void onUpdate(float dt) override;
	private:
		int mLightIdx = -1;
		Object* delegateLightRender = nullptr;
	};

	class DirectionalLightComponent : public  LightComponent {
	public:
		DirectionalLightComponent();
		void setDirection(const vec3& dir);
	private:
		int mLightIdx = -1;
	};

}

#endif