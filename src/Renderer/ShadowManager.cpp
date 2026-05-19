#include "Renderer/ShadowManager.h"
#include "Renderer/SamplerResourceManager.h"
#include "Renderer/TextureResourceMgr.h"
#include "function/Scene.h"
#include "Renderer/Light.h"

#include <algorithm>
namespace Render {
	class ShadowManagerPrivate {
	public:

		std::vector<Light*> mPointLightsToDrawShadow;
		std::vector<Light*> mDirLightsToDrawShadow;
		
	};
	ShadowManager::ShadowManager()
	{
		mDp = new ShadowManagerPrivate;
	}

	ShadowManager::~ShadowManager()
	{
		delete mDp;
	}

	void ShadowManager::setPointLightMaxCount(uint32_t count)
	{
		ShadowConfig.pointLightShadowNum = count;
	}

	void ShadowManager::setShadowTexSize(uint32_t size)
	{
		isDirShadowConfigDirty		= false;
		isPointShadowConfigDirty	= false;
		ShadowConfig.shadowRTSize = size;
	}

	void ShadowManager::setShadowTexFormat(RenderTextureFormat fmt)
	{
		isDirShadowConfigDirty = false;
		isPointShadowConfigDirty = false;
		ShadowConfig.shadowRTFormat = fmt;
	}

	void ShadowManager::drawShadow(rs_commandbuffer* cmdBuffer, Camera* currentCamera, Scene* scene)
	{
		if (isDirShadowConfigDirty) {
			prepareDirShadowResource();
			isDirShadowConfigDirty = false;
		}
	}

	void ShadowManager::drawDirLightShadow(rs_commandbuffer* cmdBuffer, Light* light, Camera* currentCamera)
	{
		//1. calculate dir light's viewproj
		// the view space of light should wrap 
		//main camera's frustum.

		// get frustum info
		// get frustum AABB
		mat4 viewMat;

	}

	void ShadowManager::processShadowDrawInfo(Scene* scene)
	{
		auto& lightMgr = scene->getLightMgr();
		auto& lightMap = lightMgr.getLightMap();
		mDp->mPointLightsToDrawShadow.clear();
		mDp->mDirLightsToDrawShadow.clear();
		for (auto& [idx, lightData] : lightMap) {
			auto light = lightData.light;
			if (!light->getHashShadow())	continue;
			if (light->getType() == LightType::Point) {
				mDp->mPointLightsToDrawShadow.push_back(light);
			}
			else {
				mDp->mDirLightsToDrawShadow.push_back(light);
			}
		};

		//TODO: Point light
		
		//Find the dir light to be main light
		if (mDp->mDirLightsToDrawShadow.size() >= 1) {
			std::nth_element(mDp->mDirLightsToDrawShadow.begin(), mDp->mDirLightsToDrawShadow.begin() + 1, mDp->mDirLightsToDrawShadow.end(), [](Light* a, Light* b) {
				return a->getIntensity() > b->getIntensity();
				});
		}

	}

	void ShadowManager::prepareDirShadowResource()
	{
		this->mDirLightShadowMap = TextureResourceManager::instance()->createRenderTexture(ShadowConfig.shadowRTFormat, ShadowConfig.shadowRTSize, ShadowConfig.shadowRTSize, 1, 1, 1, false);
	}

}