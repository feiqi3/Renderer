#include "Components/LightComponent.h"
#include "function/Scene.h"
#include "function/Object.h"
namespace Render {
	PointLightComponent::PointLightComponent()
	{
		mLight = std::make_unique<Light>(LightType::Point);
		mLight->setRange(10.f);
	}

	void PointLightComponent::onEnable()
	{
		mLight->setPosition(this->owner()->worldPosition());
		auto& lightMgr = this->owner()->scene()->getLightMgr();
		mLightIdx = lightMgr.addLight(mLight.get());
	}
	void PointLightComponent::onDisable()
	{
		auto& lightMgr = this->owner()->scene()->getLightMgr();
		if(mLightIdx >= 0)
		{
			lightMgr.removeLight(mLightIdx);
		}
	}
	void PointLightComponent::onTransformChanged()
	{
		mLight->setPosition(this->owner()->worldPosition());
	}

	void PointLightComponent::setColor(vec3 color)
	{
		mLight->setColor(color);
	}

	void PointLightComponent::setIntensity(float i)
	{
		mLight->setIntensity(i);
	}

	void PointLightComponent::setRange(float r)
	{
		mLight->setRange(r);
	}

};