#include "Renderer/ShadowManager.h"
#include "Renderer/SamplerResourceManager.h"
#include "Renderer/TextureResourceMgr.h"
#include "Renderer/CameraManager.h"
#include "function/Scene.h"
#include "Renderer/Light.h"
#include "Renderer/Camera.h"
#include "function/AABB.h"
#include <algorithm>
#include "Renderer/EnginePass.h"
#include "Renderer/ConstShaderDataManager.h"
#include "Renderer/RenderPass.h"
#include "Renderer/RenderDebuger.h"
namespace Render {
	class ShadowManagerPrivate {
	public:

		std::vector<Light*> mPointLightsToDrawShadow;
		std::vector<Light*> mDirLightsToDrawShadow;
		std::unique_ptr<Camera> mDirLightCamera = nullptr;
		rs_rendertarget* m_dirlightShadowRT = nullptr;
		u64 lastDrawDirShadowFrame = -1;
		rs_drawdata* mShadowDrawData = nullptr;
		SamplerPtr	mShadowSamplerPtr = nullptr;
		GPUShared::GPUSceneShadowData shadowData{};
	};
	ShadowManager::ShadowManager()
	{
		mDp = new ShadowManagerPrivate;
		mDp->mDirLightCamera = std::make_unique<Camera>(Name("DirLightShadowCamera"));
		CameraManager::instance()->RegisterCamera(mDp->mDirLightCamera.get(), 1);
		mDp->mShadowDrawData = RenderSystem::instance()->createDrawData();
		SamplerDesc samplerDesc{};
		samplerDesc.borderColor = BorderColor::FloatOpaqueBlack;
		samplerDesc.addressU = AddressMode::ClampToBorder;
		samplerDesc.addressV = AddressMode::ClampToBorder;
		mDp->mShadowSamplerPtr = SamplerResourceManager::instance()->getOrCreateSampler(samplerDesc);
	}

	ShadowManager::~ShadowManager()
	{
		RenderSystem::instance()->destroyRenderTarget(mDp->m_dirlightShadowRT);
		RenderSystem::instance()->destroyDrawData(mDp->mShadowDrawData);
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
		RenderMarker shadowDrawMarker(cmdBuffer, "Shadow Passes", 0.3, 0.5, 0.2, 1.);
		if (isDirShadowConfigDirty) {
			prepareDirShadowResource();
			isDirShadowConfigDirty = false;
		}
		//Begin shadowPass
		auto dirLight = scene->getLightMgr().getMainDirLight();
		bool cleanDirShadowTexture = false;
		auto dirShadowPass = RenderSystem::instance()->getRenderPass(PassName::DirectionalShadowPass);
		vec4 clearCol(1., 0., 0., 0.);
		bool dirLightShadowDrawn = false;
		if (dirLight) {
			RenderSystem::instance()->cmdClearRT(cmdBuffer, getDirShadowTexture(),mDp->m_dirlightShadowRT->m_dsView->viewKey, clearCol);
			setDirLightCamera(cmdBuffer, dirLight, mDp->mDirLightCamera.get());
			ConstShaderDataManager::instance()->updateCameraDrawData(mDp->mDirLightCamera.get());
			scene->collectVisibleObjects(mDp->mDirLightCamera.get());
			dirShadowPass->draw(cmdBuffer, mDp->mDirLightCamera.get());
			dirLightShadowDrawn = true;
		}
		else {
			//Clear this rt.
			RenderSystem::instance()->cmdClearRT(cmdBuffer, getDirShadowTexture(), mDp->m_dirlightShadowRT->m_dsView->viewKey, clearCol);
		}
		if (dirLightShadowDrawn) {
			mDp->lastDrawDirShadowFrame = RenderSystem::instance()->getNextRenderFrame();
		}

	}

	Render::rs_drawdata* ShadowManager::getShadowDrawData()const
	{
		return mDp->mShadowDrawData;
	}

	Render::TexturePtr ShadowManager::getDirShadowTexture() const
	{
		return mDirLightShadowMap;
	}

	SamplerPtr ShadowManager::getShadowSampler() const
	{
		return mDp->mShadowSamplerPtr;
	}

	void ShadowManager::setDirLightCamera(rs_commandbuffer* cmdBuffer, Light* light, Camera* currentCamera)
	{
		//1. calculate dir light's viewproj
		// the view space of light should wrap 
		// main camera's frustum.

		// get frustum info
		// get frustum AABB
		// calculate a sphere that contains the aabb
		// set camera with this

		float zDepthMin,zDepthMax;
		RenderSystem::instance()->getGlobalViewportZRange(zDepthMin, zDepthMax);
		float farZ = currentCamera->getFar();

		const float FarestDistanceDirShadow = 1000.;
		float factorOfDepthZInNDC = std::min(FarestDistanceDirShadow / farZ, 1.f);


		const vec4 NDCCoord[8] = {
			vec4(0,0,zDepthMin				,1.),
			vec4(1,0,zDepthMin				,1.),
			vec4(1,1,zDepthMin				,1.),
			vec4(0,1,zDepthMin				,1.),
			vec4(0,0,factorOfDepthZInNDC	,1.),
			vec4(1,0,factorOfDepthZInNDC	,1.),
			vec4(1,1,factorOfDepthZInNDC	,1.),
			vec4(0,1,factorOfDepthZInNDC	,1.),
		};

		AxisAlignedBoundingBox aabbOfFrustum;
		float nearPlaneZ = 0;
		float farPlaneZ	 = clamp(FarestDistanceDirShadow / currentCamera->getFar(),0.1f,1.f);
		auto viewProjOfCurCam = currentCamera->getProjectionMatrix() * currentCamera->getViewMatrix();
		auto invViewProj = inverse(viewProjOfCurCam);
		for (int i = 0;i < 8;++i) {
			vec4 PointNDCCoord = NDCCoord[i];
			PointNDCCoord.z = nearPlaneZ;
			vec4 worldPoint = invViewProj * PointNDCCoord;
			aabbOfFrustum.expand(worldPoint);
		}

		float radius = length(aabbOfFrustum.getSize()) / 2.;
		vec3 camPos = aabbOfFrustum.getCenter() - light->getDirection() * radius;
		float nearPlane = 0.01f;
		float farPlane = radius * 2.0f;
		mDp->mDirLightCamera->setTarget(aabbOfFrustum.getCenter());
		mDp->mDirLightCamera->setOrthoSize(radius);
		mDp->mDirLightCamera->setOrthographic(radius, 1., nearPlane, farPlane);

		RenderSystem::instance()->setCurrentCamera(mDp->mDirLightCamera.get());
	}

	void ShadowManager::processShadowDrawInfo(Scene* scene)
	{
		auto& lightMgr = scene->getLightMgr();
		const auto& mLightMap = lightMgr.getLightMap();
		mDp->mPointLightsToDrawShadow.clear();
		mDp->mDirLightsToDrawShadow.clear();
		for (auto& [idx, lightData] : mLightMap) {
			auto light = lightData.light;
			if (!light->getHasShadow())	continue;
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
		this->mDirLightShadowMap = TextureResourceManager::instance()->createRenderTexture(ShadowConfig.shadowRTFormat, ShadowConfig.shadowRTSize, ShadowConfig.shadowRTSize, 1, 1, 1, true);
		if (mDp->m_dirlightShadowRT) {
			RenderSystem::instance()->destroyRenderTarget(mDp->m_dirlightShadowRT);
		}
		this->mDp->m_dirlightShadowRT = RenderSystem::instance()->createRendertarget(
			{}, mDirLightShadowMap->getRsImage()
		);

		//Construct shadow info data
		auto& shadowData = mDp->shadowData;
		shadowData.DirLightShadowInfo.ViewProjMat = mDp->mDirLightCamera->getProjectionMatrix() * mDp->mDirLightCamera->getViewMatrix();

	}

}