#include "Components/LightComponent.h"
#include "function/Scene.h"
#include "function/Object.h"
#include "Components/PBRRenderComponent.h"
#include "Renderer/MaterialTemplateManager.h"
#include "Renderer/MaterialManager.h"
#include "Renderer/StandardPRBRenderEntity.h"
#include "Renderer/Materials/PBRMaterial.h"
namespace Render {

	static MaterialPtr getLightDelegateMaterial() {
		auto matTemplate = MaterialTemplateManager::instance()->getMaterialTemplate(Name("PBRMaterialTemplate_Opaque"));
		const Name lightName("Light");
		auto mat = MaterialManager::instance()->getMaterial(lightName);
		if (!mat) {
			mat = MaterialManager::instance()->createMaterial<PBRMaterial>(lightName,matTemplate);
			PBRMaterial* matPbr = (PBRMaterial*)mat.get();
			matPbr->setEmissive(vec3(55, 55, 55));
			matPbr->setBaseColor(vec4(1, 1, 1,1));
			matPbr->setRoughness(1.);
			matPbr->setMetallic (0.);
		}
		return mat;
	}

	LightComponent::LightComponent(LightType type)
	{
		mLight = std::make_unique<Light>(type);
	}

	void LightComponent::onLightDelegateEnableChanged()
	{

	}

	Render::Light* LightComponent::getLight()
	{
		return mLight.get();
	}

	PointLightComponent::PointLightComponent() : LightComponent(LightType::Point)
	{
		getLight()->setRange(10.f);
	}

	void PointLightComponent::onLightDelegateEnableChanged()
	{
		if (!mLightDelegate && delegateLightRender) {
			this->owner()->scene()->destroyObject(delegateLightRender);
		}

		if (mLightDelegate) {
			delegateLightRender = this->owner()->scene()->createObject("LightDelegatePoint");
			delegateLightRender->setParent(this->owner());
			auto comp = delegateLightRender->addComponent<PBRRenderComponent>();
			comp->setMesh(
				ResourceSystem::instance()->getResource<Mesh>(
					Mesh::typeName(), Name("Builtin::Sphere")
				)
			);
			comp->setMaterial(0, getLightDelegateMaterial());
			comp->setCastShadow(false);
		}
	}

	void PointLightComponent::onDisable()
	{
		LightComponent::onDisable();
		delegateLightRender->getComponent<PBRRenderComponent>()->setEnabled(false);
	}

	void PointLightComponent::onEnable()
	{
		LightComponent::onEnable();
		delegateLightRender->getComponent<PBRRenderComponent>()->setEnabled(true);
	}

	void LightComponent::onEnable()
	{
		getLight()->setPosition(this->owner()->worldPosition());
		auto& lightMgr = this->owner()->scene()->getLightMgr();
		mLightIdx = lightMgr.addLight(mLight.get());
		onLightDelegateEnableChanged();
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
		bool triggerChanged = false;
		if (mLightDelegate != d) {
			triggerChanged = true;
		}
		mLightDelegate = d;
		if (triggerChanged) {
			this->onLightDelegateEnableChanged();
		}
	}

	void LightComponent::setIntensity(float i)
	{
		getLight()->setIntensity(i);
	}

	void PointLightComponent::setRange(float r)
	{
		getLight()->setRange(r);
	}

	void PointLightComponent::onUpdate(float dt)
	{
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