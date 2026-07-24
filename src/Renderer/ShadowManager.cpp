#include "Renderer/ShadowManager.h"
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
#include <algorithm>

namespace Render {

	class ShadowManagerPrivate {
	public:
		std::unique_ptr<CascadedShadow> mCascadedShadow = nullptr;

		std::vector<Light*> mPointLightsToDrawShadow;
		std::vector<Light*> mDirLightsToDrawShadow;
		int                 mPointLightShadowNum = 0;

		rs_drawdata* mShadowDrawData = nullptr;
		SamplerPtr   mShadowSamplerPtr = nullptr;
	};

	ShadowManager::ShadowManager() {
		mDp = new ShadowManagerPrivate();

		mDp->mCascadedShadow = std::make_unique<CascadedShadow>();

		mDp->mShadowDrawData = RenderSystem::instance()->createDrawData();

		SamplerDesc samplerDesc{};
		samplerDesc.minFilter = Filter::Linear;
		samplerDesc.magFilter = Filter::Linear;
		samplerDesc.borderColor = BorderColor::FloatOpaqueBlack;
		samplerDesc.addressU = AddressMode::ClampToBorder;
		samplerDesc.addressV = AddressMode::ClampToBorder;
		mDp->mShadowSamplerPtr = SamplerResourceManager::instance()->getOrCreateSampler(samplerDesc);
	}

	ShadowManager::~ShadowManager() {
		RenderSystem::instance()->destroyDrawData(mDp->mShadowDrawData);
		delete mDp;
	}

	void ShadowManager::setShadowEnable(bool isEnable) {
		mIsShadowEnable = isEnable;
		if (mDp->mCascadedShadow) {
			mDp->mCascadedShadow->setShadowEnable(isEnable);
		}
	}

	bool ShadowManager::getShadowEnable() const {
		return mIsShadowEnable;
	}

	void ShadowManager::setShadowTechnique(ShadowTechnique tech) {
		mTechnique = tech;
	}

	ShadowManager::ShadowTechnique ShadowManager::getShadowTechnique() const {
		return mTechnique;
	}

	void ShadowManager::setShadowTexSize(uint32_t size) {
		if (mDp->mCascadedShadow) {
			mDp->mCascadedShadow->setTextureSize(size);
		}
	}

	void ShadowManager::setShadowTexFormat(RenderTextureFormat fmt) {
	}

	void ShadowManager::setShadowCameraHeight(float h) {
		if (mDp->mCascadedShadow) {
			mDp->mCascadedShadow->setCameraHeight(h);
		}
	}

	void ShadowManager::setCascadedLayers(int layers) {
		if (mDp->mCascadedShadow) {
			mDp->mCascadedShadow->setCascadedLayers(layers);
		}
	}

	int ShadowManager::getCascadedLayers() const {
		return mDp->mCascadedShadow ? mDp->mCascadedShadow->getCascadedLayers() : 0;
	}

	void ShadowManager::setCascadedLayerDistance(int layer, float dis) {
		if (mDp->mCascadedShadow) {
			mDp->mCascadedShadow->setCascadedLayerDistance(layer, dis);
		}
	}

	float ShadowManager::getCascadedLayerDistance(int layer) const {
		return mDp->mCascadedShadow ? mDp->mCascadedShadow->getCascadedLayerDistance(layer) : 0.0f;
	}

	void ShadowManager::setCascadedInterpolateFactor(float x) {
		if (mDp->mCascadedShadow) {
			mDp->mCascadedShadow->setCascadedInterpolateFactor(x);
		}
	}

	float ShadowManager::getCascadedInterpolateFactor() const {
		return mDp->mCascadedShadow ? mDp->mCascadedShadow->getCascadedInterpolateFactor() : 0.0f;
	}

	void ShadowManager::setPointLightMaxCount(uint32_t count) {
		mDp->mPointLightShadowNum = count;
	}

	CascadedShadow* ShadowManager::getCascadedShadow() const {
		return mDp->mCascadedShadow.get();
	}

	TexturePtr ShadowManager::getDirShadowTexture() const {
		return mDp->mCascadedShadow ? mDp->mCascadedShadow->getShadowTexture() : nullptr;
	}

	SamplerPtr ShadowManager::getShadowSampler() const {
		return mDp->mShadowSamplerPtr;
	}

	rs_drawdata* ShadowManager::getShadowDrawData() const {
		return mDp->mShadowDrawData;
	}

	GPUShared::GPUSceneShadowData ShadowManager::getSceneShadowData() const {
		GPUShared::GPUSceneShadowData shadowData{};

		if (mDp->mCascadedShadow) {
			shadowData.DirLightShadowInfo = mDp->mCascadedShadow->getShadowData();
		}

		shadowData.ShadowInfo.x = getShadowEnable() ? 1.0f : -1.0f;
		switch (mTechnique) {
		case ShadowTechnique::Normal:
			shadowData.ShadowInfo.y = 0.0f;
			break;
		case ShadowTechnique::PCF:
			shadowData.ShadowInfo.y = 1.0f;
			break;
		default:
			break;
		}

		return shadowData;
	}

	void ShadowManager::drawShadow(rs_commandbuffer* cmdBuffer, Camera* currentCamera, Scene* scene) {
		mDp->mCascadedShadow->prepareShadowResources();
		if (!getShadowEnable() || !scene) return;

		RenderMarker shadowDrawMarker(cmdBuffer, "Shadow Passes", 0.3f, 0.5f, 0.2f, 1.0f);

		if (mDp->mCascadedShadow) {
			mDp->mCascadedShadow->draw(cmdBuffer, currentCamera, scene);
		}
	}

}