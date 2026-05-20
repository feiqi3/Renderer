#include "Renderer/ShadowManager.h"
#include "Renderer/SamplerResourceManager.h"
#include "Renderer/TextureResourceMgr.h"
#include "Renderer/CameraManager.h"
#include "function/Scene.h"
#include "Renderer/Light.h"
#include "Renderer/Camera.h"
#include "function/AABB.h"
#include <algorithm>
namespace Render {
	class ShadowManagerPrivate {
	public:

		std::vector<Light*> mPointLightsToDrawShadow;
		std::vector<Light*> mDirLightsToDrawShadow;

		std::unique_ptr<Camera> mDirLightCamera = nullptr;
		
	};
	ShadowManager::ShadowManager()
	{
		mDp = new ShadowManagerPrivate;
		mDp->mDirLightCamera = std::make_unique<Camera>(Name("DirLightShadowCamera"));
		CameraManager::instance()->RegisterCamera(mDp->mDirLightCamera.get(), 1);
	}

	ShadowManager::~ShadowManager()
	{
		CameraManager::instance()->UnregisterCamera(mDp->mDirLightCamera.get());
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

	void ShadowManager::setDirLightCamera(rs_commandbuffer* cmdBuffer, Light* light, Camera* currentCamera)
	{
		//1. calculate dir light's viewproj
		// the view space of light should wrap 
		//main camera's frustum.

		// get frustum info
		// get frustum AABB
		// calculate a sphere that contains the aabb
		// set camera with this 
		const vec4 NDCCoord[] = {
			vec4(0,0,0,1.),
			vec4(1,0,0,1.),
			vec4(1,1,0,1.),
			vec4(0,1,0,1.),
		};

		const float FarestDistanceDirShadow = 1000.;

		AxisAlignedBoundingBox aabbOfFrustum;
		float nearPlaneZ =	0;
		float farPlaneZ =	clamp(FarestDistanceDirShadow / currentCamera->getFar(),0.1,1.);
		auto viewProjOfCurCam = currentCamera->getProjectionMatrix() * currentCamera->getViewMatrix();
		auto invViewProj = inverse(viewProjOfCurCam);
		for (int i = 0;i < 4;++i) {
			vec4 NDCCoordNear = NDCCoord[i];
			NDCCoordNear.z = nearPlaneZ;
			vec4 nearPlanePoint = invViewProj * NDCCoordNear;
			aabbOfFrustum.expand(nearPlanePoint);
		}
		for (int i = 0;i < 4;++i) {
			vec4 NDCCoordFar = NDCCoord[i];
			NDCCoordFar.z = farPlaneZ;
			vec4 farPlanePoint = invViewProj * NDCCoordFar;
			aabbOfFrustum.expand(farPlanePoint);
		}

		float radius = length(aabbOfFrustum.getCenter() - aabbOfFrustum.getMax());
		vec3 camPos = aabbOfFrustum.getCenter() - light->getDirection() * radius;
		float nearPlane = 0.0f;
		float farPlane = radius * 2.0f;
		mDp->mDirLightCamera->setTarget(aabbOfFrustum.getCenter());
		mDp->mDirLightCamera->setOrthoSize(radius);
		mDp->mDirLightCamera->setOrthographic(radius, 1., nearPlane, farPlane);

		RenderSystem::instance()->setCurrentCamera(mDp->mDirLightCamera.get());
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