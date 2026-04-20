#include "Components/SkyboxRenderComponent.h"
#include "Renderer/SkyboxRenderEntity.h"
#include "Renderer/RenderQueue.h"
#include "function/Object.h"
#include "function/Scene.h"
#include "Renderer/LightManager.h"
namespace Render {
	Render::SkyboxRenderComponent::SkyboxRenderComponent()
	{
		this->mEntity = new SkyBoxRenderEntity();
		mData.rotateQuat			= vec4(0., 0., 0., 1.);
		mData.colorexposureTuning	= vec4(1.);
	}
	SkyboxRenderComponent::~SkyboxRenderComponent()
	{
		delete mEntity;
		mEntity = 0;
	}

	void SkyboxRenderComponent::setSkybox(TexturePtr texture)
	{
		mSkyboxCubeMap = texture;
		mDataDirty = true;
		this->owner()->scene()->getLightMgr().setSkybox(texture);
	}

	void SkyboxRenderComponent::setSampler(SamplerPtr sampler)
	{
		mSkyboxSampler = sampler;
		mDataDirty = true;
	}

	void SkyboxRenderComponent::setRotation(const quat& q)
	{
		mData.rotateQuat = vec4(q.x, q.y, q.z, q.w);
		mDataDirty = true;
	}

	void SkyboxRenderComponent::setExposure(float exposure)
	{
		mData.colorexposureTuning.w = exposure;
		mDataDirty = true;
	}

	void SkyboxRenderComponent::setColor(vec3 color)
	{
		mData.colorexposureTuning.x = color.x;
		mData.colorexposureTuning.y = color.y;
		mData.colorexposureTuning.z = color.z;
		mDataDirty = true;
	}

	void SkyboxRenderComponent::onUpdate(float dt)
	{
		if (mDataDirty) {
			mDataDirty = false;
			this->mEntity->setSkyboxCubemap(mSkyboxCubeMap, mSkyboxSampler);
			this->mEntity->setGPUData(mData);
		}
		RenderSystem::instance()->getMainRenderQueue()->submit(
			mEntity, RenderMask::SkyBox
		);
	}

}

