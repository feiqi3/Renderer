#ifndef LIGHT_COMPONENT_H_
#define LIGHT_COMPONENT_H_

#include "Renderer/Light.h"
#include "function/Component.h"
#include "common/CommonMath.h"
#include <memory>
namespace Render {
	class PointLightComponent : public  Component {
	public:
		PointLightComponent();
		void onEnable() override;
		void onDisable() override;
		void onTransformChanged() override;

		void setColor(vec3 color);
		void setIntensity(float i);
		void setRange(float r);
	private:
		std::unique_ptr<Light> mLight = nullptr;
		int mLightIdx = -1;
	};
}

#endif