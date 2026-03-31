#ifndef LIGHT_COMPONENT_H_
#define LIGHT_COMPONENT_H_

#include "Renderer/Light.h"
#include "function/Component.h"
#include "common/CommonMath.h"
#include <memory>
namespace Render {
	class LightComponent : public  Component {
	public:
		inline LightComponent(LightType type);
		Light* getLight();
		void onEnable() override;
		void onDisable() override;
		void onTransformChanged() override;
		void setColor(vec3 color);
		void setIntensity(float i);

	protected:
		std::unique_ptr<Light> mLight = nullptr;
		int mLightIdx = -1;
	};

	class PointLightComponent : public  LightComponent {
	public:
		PointLightComponent();

		void setRange(float r);
	private:
		int mLightIdx = -1;
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