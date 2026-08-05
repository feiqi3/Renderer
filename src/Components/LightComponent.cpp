#include "Components/LightComponent.h"
#include "function/Scene.h"
#include "function/Object.h"
namespace Render {


	LightComponent::LightComponent(LightType type)
	{
		mLight = std::make_unique<Light>(type);
	}

	Render::Light* LightComponent::getLight()
	{
		return mLight.get();
	}

	PointLightComponent::PointLightComponent() : LightComponent(LightType::Point)
	{
		getLight()->setRange(10.f);
	}

	void LightComponent::onEnable()
	{
		getLight()->setPosition(this->owner()->worldPosition());
		auto& lightMgr = this->owner()->scene()->getLightMgr();
		mLightIdx = lightMgr.addLight(mLight.get());
	}
	void LightComponent::onDisable()
	{
		auto& lightMgr = this->owner()->scene()->getLightMgr();
		if(mLightIdx >= 0)
		{
			lightMgr.removeLight(mLightIdx);
		}
	}
	void LightComponent::onTransformChanged()
	{
		getLight()->setPosition(this->owner()->worldPosition());
	}

	void LightComponent::setColor(vec3 color)
	{
		getLight()->setColor(color);
	}

	void LightComponent::setEnableLightDelegate(bool d)
	{
		mLightDelegate = d;
	}

	void LightComponent::setIntensity(float i)
	{
		getLight()->setIntensity(i);
	}

	void PointLightComponent::setRange(float r)
	{
		getLight()->setRange(r);
	}

	DirectionalLightComponent::DirectionalLightComponent() : LightComponent(LightType::Directional)
	{
		setDirection(vec3(1., 1., 0.));
	}

	void DirectionalLightComponent::setDirection(const vec3& dir)
	{
		//Light direction is set as the vector from surface to light direction, but to make it easy to understand in component
		//I take a minus dir as parameter......
		getLight()->setDirection(-dir);
	}


};