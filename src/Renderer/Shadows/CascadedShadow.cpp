#include "Renderer/Shadows/CascadedShadow.h"
#include "Renderer/SamplerResourceManager.h"
#include "Renderer/TextureResourceMgr.h"
#include "Renderer/CameraManager.h"
#include "function/Scene.h"
#include "Renderer/Light.h"
#include "Renderer/Camera.h"
#include "function/AABB.h"
#include "Renderer/EnginePass.h"
#include "Renderer/ConstShaderDataManager.h"
#include "Renderer/RenderPass.h"
#include "Renderer/RenderDebuger.h"
#include "function/CullUtils.h"
#include <algorithm>
#include <vector>

namespace Render {

	class CascadedShadowPrivate {
	public:
		std::vector<std::unique_ptr<Camera>> mCascadeCameras;
		rs_rendertarget* m_dirlightShadowRT[MAX_CASCADED_LAYERS] = {nullptr};

		float dirLightCameraHeight = 100.0f;
	};

	CascadedShadow::CascadedShadow() {
		mDp = new CascadedShadowPrivate();

		for (int i = 0; i < MAX_CASCADE_LAYERS; ++i) {
			auto cam = std::make_unique<Camera>(Name("CSM_LightCamera_" + std::to_string(i)));
			CameraManager::instance()->RegisterCamera(cam.get(), 1);
			mDp->mCascadeCameras.push_back(std::move(cam));
		}


		SamplerDesc samplerDesc{};
		samplerDesc.minFilter = Filter::Linear;
		samplerDesc.magFilter = Filter::Linear;
		samplerDesc.borderColor = BorderColor::FloatOpaqueBlack;
		samplerDesc.addressU = AddressMode::ClampToBorder;
		samplerDesc.addressV = AddressMode::ClampToBorder;
	}

	CascadedShadow::~CascadedShadow() {
		for (int i = 0;i < MAX_CASCADE_LAYERS;++i) {
			if(mDp->m_dirlightShadowRT[i])
			RenderSystem::instance()->destroyRenderTarget(mDp->m_dirlightShadowRT[i]);
		}
		for (auto& cam : mDp->mCascadeCameras) {
			CameraManager::instance()->UnregisterCamera(cam.get());
		}

		delete mDp;
	}

	void CascadedShadow::setCascadedLayers(int layers) {
		int newLayers = std::clamp(layers, 1, MAX_CASCADE_LAYERS);
		if (mCascadedLayers != newLayers) {
			mCascadedLayers = newLayers;
			mIsResourceDirty = true;
		}
	}

	void CascadedShadow::setCascadedLayerDistance(int layer, float dis) {
		if (layer >= 0 && layer < MAX_CASCADE_LAYERS) {
			mCascadedDistance[layer] = dis;
		}
	}

	float CascadedShadow::getCascadedLayerDistance(int layer) const {
		if (layer >= 0 && layer < MAX_CASCADE_LAYERS) {
			return mCascadedDistance[layer];
		}
		return 0.0f;
	}

	void CascadedShadow::setCascadedInterpolateFactor(float x) {
		mBlendFactor = std::clamp(x, 0.0f, 1.0f);
	}

	void CascadedShadow::setCascadedProjectionCullSize(int layer, float x)
	{
		if (layer >= 0 && layer < MAX_CASCADE_LAYERS) {
			mCascadedCullProjectionSize[layer] = x;
		}
	}

	float CascadedShadow::getCascadedProjectionCullSize(int layer)
	{
		if (layer >= 0 && layer < MAX_CASCADE_LAYERS) {
			return mCascadedCullProjectionSize[layer];
		}
	}

	void CascadedShadow::setTextureSize(uint32_t size) {
		if (mTextureSize != size) {
			mTextureSize = size;
			mIsResourceDirty = true;
		}
	}

	void CascadedShadow::setShadowEnable(bool enable) {
		if (enable != mIsEnable)mIsResourceDirty = true;
		mIsEnable = enable;
	}

	void CascadedShadow::setCameraHeight(float f)
	{
		mDp->dirLightCameraHeight = f;
	}

	float CascadedShadow::getCameraHeight() const
	{
		return mDp->dirLightCameraHeight;
	}

	TexturePtr CascadedShadow::getShadowTexture() const {
		return mShadowMap;
	}

	void CascadedShadow::prepareShadowResources() {
		if (!mIsResourceDirty) return;

		if (mIsEnable) {
			mShadowMap = TextureResourceManager::instance()->createRenderTexture(
				RenderTextureFormat::D32,
				mTextureSize,
				mTextureSize,
				1,
				1, 
				mCascadedLayers,
				true
			);
		}
		else {
			mShadowMap = TextureResourceManager::instance()->createRenderTexture(
				RenderTextureFormat::D32, 4, 4, 1, 1, 1,true
			);
		}

		for (int i = 0;i < MAX_CASCADE_LAYERS;++i) {
			if (mDp->m_dirlightShadowRT[i]){
				RenderSystem::instance()->destroyRenderTarget(mDp->m_dirlightShadowRT[i]);
			}
		}
		std::vector<rs_image*> imageRT = { mShadowMap->getRsImage() };
		std::vector<ImageViewKey> imageRTView = { ImageViewKey() };
		for (int i = 0;i < mCascadedLayers;++i) {
			imageRTView[0].setBaseLayer(i).setLayerCount(1).setAspect(ViewAspect::Depth);
			mDp->m_dirlightShadowRT[i] =
				RenderSystem::instance()->createRendertargetDetailed(imageRT, imageRTView ,true);
		}

		mIsResourceDirty = false;
	}

	void CascadedShadow::updateCascadeCameras(Camera* mainCamera, Light* mainDirLight) {
		if (!mainCamera || !mainDirLight) return;

		float zDepthMin, zDepthMax;
		RenderSystem::instance()->getGlobalViewportZRange(zDepthMin, zDepthMax);
		vec3 lightDir = normalize(mainDirLight->getDirection());

		vec3 upHint = (std::abs(dot(lightDir, vec3(0.0f, 1.0f, 0.0f))) > 0.999f)
			? vec3(0.0f, 0.0f, 1.0f) : vec3(0.0f, 1.0f, 0.0f);
		vec3 zAxis = lightDir;                          
		vec3 xAxis = normalize(cross(upHint, zAxis));   
		vec3 yAxis = normalize(cross(zAxis, xAxis));

		const auto& proj = mainCamera->getProjectionMatrix();
		const auto& view = mainCamera->getViewMatrix();
		auto invViewProj = inverse(proj * view);
		for (int i = 0; i < mCascadedLayers; ++i) {
			float nearZ = (i == 0) ? mainCamera->getNear() : mCascadedDistance[i - 1];
			float farZ = mCascadedDistance[i];

			vec4 vn = proj * vec4(0.0f, 0.0f, -nearZ, 1.0f);
			vec4 vf = proj * vec4(0.0f, 0.0f, -farZ, 1.0f);
			float ndcNearZ = vn.z / vn.w;
			float ndcFarZ = vf.z / vf.w;

			const vec4 NDCCoord[8] = {
				vec4(-1.0f, -1.0f, ndcNearZ, 1.0f), vec4(1.0f, -1.0f, ndcNearZ, 1.0f),
				vec4(1.0f,  1.0f, ndcNearZ, 1.0f), vec4(-1.0f,  1.0f, ndcNearZ, 1.0f),
				vec4(-1.0f, -1.0f, ndcFarZ , 1.0f), vec4(1.0f, -1.0f, ndcFarZ , 1.0f),
				vec4(1.0f,  1.0f, ndcFarZ , 1.0f), vec4(-1.0f,  1.0f, ndcFarZ , 1.0f)
			};

			vec3 vMin(std::numeric_limits<float>::max());
			vec3 vMax(-std::numeric_limits<float>::max());
			for (int j = 0; j < 8; ++j) {
				vec4 wp = invViewProj * NDCCoord[j];
				wp /= wp.w;
				vec3 lp = vec3(
					dot(vec3(wp), xAxis),
					dot(vec3(wp), yAxis),
					dot(vec3(wp), zAxis)
				);
				vMin = min(vMin, lp);
				vMax = max(vMax, lp);
			}

			float halfExtent = std::max(vMax.x - vMin.x, vMax.y - vMin.y) * 0.5f;

			float cx = (vMin.x + vMax.x) * 0.5f;
			float cy = (vMin.y + vMax.y) * 0.5f;
			float cz = (vMin.z + vMax.z) * 0.5f;
			float worldUnitsPerTexel = (halfExtent * 2.0f) / static_cast<float>(mTextureSize);
			cx = std::floor(cx / worldUnitsPerTexel) * worldUnitsPerTexel;
			cy = std::floor(cy / worldUnitsPerTexel) * worldUnitsPerTexel;

			vec3 target = xAxis * cx + yAxis * cy + zAxis * cz;
			vec3 lightPos = target + lightDir * mDp->dirLightCameraHeight;

			Camera* cascadeCam = mDp->mCascadeCameras[i].get();
			cascadeCam->setPosition(lightPos);
			cascadeCam->setTarget(target);
			cascadeCam->setOrthoSize(halfExtent);
			cascadeCam->setOrthographic(halfExtent, 1.0f,
				0.01f, mDp->dirLightCameraHeight * 2.0f);
		}

		updateDirlightShaderData(mainDirLight);
	}

	void CascadedShadow::updateDirlightShaderData(Light* light)
	{
		for (int i = 0;i < MAX_CASCADE_LAYERS;++i) {
			//???
			auto cam = mDp->mCascadeCameras[i].get();
			auto& dirCascadeData = mShadowData.cascadedShadowData[i];
			dirCascadeData.ViewMat = cam->getViewMatrix();
			dirCascadeData.ProjMat = cam->getProjectionMatrix();
			dirCascadeData.Info.x = getCascadedLayerDistance(i);
			dirCascadeData.OrthoSize.x = cam->getOrthoSize();
			dirCascadeData.OrthoSize.y = cam->getOrthoSize() / cam->getAspectRatio();
		}
	
		mShadowData.AtlasInfo.x = getTextureSize();
		mShadowData.AtlasInfo.y = getTextureSize();
		mShadowData.AtlasInfo.z = mCascadedLayers;
		mShadowData.AtlasInfo.w = getCascadedInterpolateFactor();
	
		mShadowData.LightDir = vec4(light->getDirection(),1.0);
	}

	void CascadedShadow::draw(rs_commandbuffer* cmdBuffer, Camera* mainCamera, Scene* scene) {
		prepareShadowResources();

		if (!mIsEnable || !scene) return;

		auto mainDirLight = scene->getLightMgr().getMainDirLight();
		if (!mainDirLight) {
			vec4 clearCol(1.0f, 0.0f, 0.0f, 0.0f);
			auto layers = this->mShadowMap->getRsImage()->arrayLayers;
			auto viewKey = ImageViewKey();
			viewKey.setBaseLayer(0).setLayerCount(1);
			RenderSystem::instance()->cmdClearRT(cmdBuffer, getShadowTexture(), viewKey, clearCol);
			return;
		}

		RenderMarker shadowDrawMarker(cmdBuffer, "CSM Passes", 0.3f, 0.5f, 0.2f, 1.0f);

		auto dirShadowPass = RenderSystem::instance()->getRenderPass(PassName::DirectionalShadowPass);

		vec3 lightDir = mainDirLight->getDirection();
		auto funcCull = [lightDir](float cullSize, Camera* cam,const Frustum& frustum ,const AxisAlignedBoundingBox& aabb)->bool {
			//1. aabb's projected size in shadow camera 
			auto size = aabb.getSize();
			auto longEdge = aabb.getLongestEdge();
			//Get long edge's projection in light dir
			float cosTheta = dot(longEdge, lightDir);
			float sinTheta = 1 - cosTheta * cosTheta;
			float projectionOnLightCamPlane = sinTheta * dot(longEdge, size);
			if (projectionOnLightCamPlane < cullSize)
			{
				//Cull by projection size
				return false;
			}

			//2. frustum cull
			return frustum.isVisible(aabb);

		};

		updateCascadeCameras(mainCamera, mainDirLight);

		for (int i = 0;i < mCascadedLayers;++i) {
			auto cam = mDp->mCascadeCameras[i].get();
			ConstShaderDataManager::instance()->updateCameraDrawData(cam);
			//1. collect all visible objects
			auto cullFuncWithCam = std::bind(funcCull, mCascadedCullProjectionSize[i], cam, std::placeholders::_1, std::placeholders::_2);
			scene->collectVisibleObjects(cam, cullFuncWithCam);
			//2. send to draw.
			dirShadowPass->setRenderTarget(mDp->m_dirlightShadowRT[i]);
			std::string markerStr = "Cascaded layer " + std::to_string(i);
			RenderMarker marker(cmdBuffer, markerStr.c_str(), 0.25, 1, 0.25, 1);
			dirShadowPass->draw(cmdBuffer, mDp->mCascadeCameras[i].get());
		}

	}

}