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
		samplerDesc.minFilter = Filter::Linear;
		samplerDesc.magFilter = Filter::Linear;
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
		isDirShadowRTDirty			= true;
		isDirShadowConfigDirty		= true;
		isPointShadowConfigDirty	= true;
		ShadowConfig.shadowRTSize = size;
	}

	void ShadowManager::setShadowTexFormat(RenderTextureFormat fmt)
	{
		isDirShadowRTDirty			= true;
		isDirShadowConfigDirty		= true;
		isPointShadowConfigDirty	= true;
		ShadowConfig.shadowRTFormat = fmt;
	}

	void ShadowManager::setShadowCameraHeight(float h)
	{
		ShadowConfig.dirLightCameraHeight = h;
	}

	void ShadowManager::setDirLightShadowFarZ(float f)
	{
		ShadowConfig.dirLightShadowFarZ = f;
	}

	bool ShadowManager::getShadowEnable() const
	{
		return isShadowEnable;
	}

	void ShadowManager::setShadowEnable(bool isEnable)
	{
		if (isShadowEnable != isEnable)isDirShadowConfigDirty = true;
		isShadowEnable = isEnable;
	}

	Render::ShadowManager::ShadowTechnique ShadowManager::getShadowTechnique() const
	{
		return mTechnique;
	}

	void ShadowManager::setShadowTechnique(ShadowTechnique tech)
	{
		if (tech != getShadowTechnique()) {
			isDirShadowConfigDirty = true;
		}
		mTechnique = tech;
	}

	void ShadowManager::drawShadow(rs_commandbuffer* cmdBuffer, Camera* currentCamera, Scene* scene)
	{
		if (isDirShadowConfigDirty) {
			prepareDirShadowResource();

			if ( !isShadowEnable ) {
				RenderSystem::instance()->cmdClearRT(cmdBuffer, mDirLightShadowMap, mDirLightShadowMap->getRsImage()->defaultView->viewKey, vec4(1.0, 1.0, 1.0, 1.0));
			}

			isDirShadowConfigDirty = false;
		}
		if (!getShadowEnable())return;
		RenderMarker shadowDrawMarker(cmdBuffer, "Shadow Passes", 0.3, 0.5, 0.2, 1.);
		//Begin shadowPass
		auto dirLight = scene->getLightMgr().getMainDirLight();
		bool cleanDirShadowTexture = false;
		auto dirShadowPass = RenderSystem::instance()->getRenderPass(PassName::DirectionalShadowPass);
		vec4 clearCol(1., 0., 0., 0.);
		bool dirLightShadowDrawn = false;
		if (dirLight) {
			setDirLightCamera(cmdBuffer, dirLight, currentCamera);
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

	GPUShared::GPUSceneShadowData ShadowManager::getSceneShadowData() const
	{
		GPUShared::GPUSceneShadowData shadowData{};
		shadowData.DirLightShadowInfo.ViewMat = mDp->mDirLightCamera->getViewMatrix();
		shadowData.DirLightShadowInfo.ProjMat = mDp->mDirLightCamera->getProjectionMatrix();
		shadowData.DirLightShadowInfo.AtlasInfo.x = ShadowConfig.shadowRTSize;
		shadowData.DirLightShadowInfo.AtlasInfo.y = ShadowConfig.shadowRTSize;
		shadowData.ShadowInfo.x = getShadowEnable() ? 1 : -1;
		switch (mTechnique)
		{
		case Render::ShadowManager::ShadowTechnique::Normal:
			shadowData.ShadowInfo.y = 0.;
			break;
		case Render::ShadowManager::ShadowTechnique::PCF:
			shadowData.ShadowInfo.y = 1.;
			break;
		default:
			break;
		}
		return shadowData;
	}

	void ShadowManager::setDirLightCamera(rs_commandbuffer* cmdBuffer, Light* light, Camera* currentCamera)
	{
		// calculate dir light's viewproj
		// the view space of light should wrap 
		// main camera's frustum.

		// get frustum info
		// get frustum AABB
		// calculate a sphere that contains the aabb
		// set camera with this
		//-----------------------------------------------------------//
		// v2 //
		// Camera rotate invoke shadow camera boundary size change in world space, cause jitter or other artifacts  
		// So what i gonna do is to calculate the aabb inside camera space(main cam), and then the boundary sphere's r 
		// is the target size of aabb, then translate the center point into world space.

		float zDepthMin, zDepthMax;
		RenderSystem::instance()->getGlobalViewportZRange(zDepthMin, zDepthMax);

		float farZ = currentCamera->getFar();
		const float FarestDistanceDirShadow = ShadowConfig.dirLightShadowFarZ;
		float shadowFarZ = std::min(FarestDistanceDirShadow, farZ);

		vec4 viewPointFar(0.0f, 0.0f, -shadowFarZ, 1.0f);
		vec4 clipPointFar = currentCamera->getProjectionMatrix() * viewPointFar;
		float ndcFarZ = clipPointFar.z / clipPointFar.w;

		float ndcNearZ = zDepthMin; 

		const vec4 NDCCoord[8] = {
			vec4(-1.0f, -1.0f, ndcNearZ, 1.0f),
			vec4(1.0f, -1.0f, ndcNearZ, 1.0f),
			vec4(1.0f,  1.0f, ndcNearZ, 1.0f),
			vec4(-1.0f,  1.0f, ndcNearZ, 1.0f),
			vec4(-1.0f, -1.0f, ndcFarZ,  1.0f),
			vec4(1.0f, -1.0f, ndcFarZ,  1.0f),
			vec4(1.0f,  1.0f, ndcFarZ,  1.0f),
			vec4(-1.0f,  1.0f, ndcFarZ,  1.0f)
		};

		vec4 worldSpacePoints[8] = {};

		AxisAlignedBoundingBox aabbOfFrustum;
		AxisAlignedBoundingBox aabbOfWorldFrustum;
		float farPlaneZ = clamp(FarestDistanceDirShadow / currentCamera->getFar(), 0.1f, 1.f);
		auto viewProjOfCurCam = currentCamera->getProjectionMatrix() * currentCamera->getViewMatrix();
		auto invViewProj = inverse(viewProjOfCurCam);
		for (int i = 0;i < 8;++i) {
			vec4 PointNDCCoord = NDCCoord[i];
			vec4 worldPoint = invViewProj * PointNDCCoord;
			worldPoint = worldPoint / worldPoint.w;
			worldSpacePoints[i] = worldPoint;
			aabbOfWorldFrustum.expand(worldPoint);
			vec4 mainCamSpacePoint = currentCamera->getViewMatrix() * worldSpacePoints[i];
			aabbOfFrustum.expand(mainCamSpacePoint);
		}

		vec3 centerOfAABBInWorld = aabbOfWorldFrustum.getCenter();
		auto sizeOfFrustum = aabbOfFrustum.getSize();
		float radiusOfFrustum = std::max(sizeOfFrustum.x, sizeOfFrustum.z);
		auto positionOfLightSourceInWorld = light->getDirection() * ShadowConfig.dirLightCameraHeight + centerOfAABBInWorld;
		float nearPlane = 0.01f;
		float farPlane = ShadowConfig.dirLightCameraHeight;
		mDp->mDirLightCamera->setPosition(positionOfLightSourceInWorld);
		mDp->mDirLightCamera->setTarget(centerOfAABBInWorld);
		mDp->mDirLightCamera->setOrthoSize(radiusOfFrustum);
		mDp->mDirLightCamera->setOrthographic(radiusOfFrustum, 1., nearPlane, farPlane);

		//float radius = length(aabbOfFrustum.getSize()) / 2.;
		//vec4 worldCenterOfBox = inverse(currentCamera->getViewMatrix()) * vec4(aabbOfFrustum.getCenter(), 1.);
		//vec3 worldPosOfShadowCamera = vec3(worldCenterOfBox) + light->getDirection() * ShadowConfig.dirLightCameraHeight;
		//vec3 camPos = worldPosOfShadowCamera;
		////DebugDrawManager::instance()->drawAABB(aabbOfFrustum,vec4(1,.0,0,0.3));
		//float nearPlane = 0.01f;
		//float farPlane = radius * 2.0f + ShadowConfig.dirLightCameraHeight;
		//mDp->mDirLightCamera->setPosition(camPos);
		//mDp->mDirLightCamera->setTarget(worldCenterOfBox);
		//mDp->mDirLightCamera->setOrthoSize(radius);
		//mDp->mDirLightCamera->setOrthographic(radius, 1., nearPlane, farPlane);

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
		if (isDirShadowRTDirty) {
			if (getShadowEnable()) {
				this->mDirLightShadowMap = TextureResourceManager::instance()->createRenderTexture(ShadowConfig.shadowRTFormat, ShadowConfig.shadowRTSize, ShadowConfig.shadowRTSize, 1, 1, 1, true);
			}
			else {
				//Fake image
				this->mDirLightShadowMap = TextureResourceManager::instance()->createRenderTexture(ShadowConfig.shadowRTFormat, 4, 4, 1, 1, 1, true);
			}

			if (mDp->m_dirlightShadowRT) {
				RenderSystem::instance()->destroyRenderTarget(mDp->m_dirlightShadowRT);
			}
			this->mDp->m_dirlightShadowRT = RenderSystem::instance()->createRendertarget(
				{}, mDirLightShadowMap->getRsImage()
			);

			//Construct shadow info data
			RenderSystem::instance()->getRenderPass(PassName::DirectionalShadowPass)->setRenderTarget(mDp->m_dirlightShadowRT);
			isDirShadowRTDirty = false;
		}
	}

}