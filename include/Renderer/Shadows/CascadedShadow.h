#ifndef CASCADED_SHADOW_H_
#define CASCADED_SHADOW_H_

#include "Renderer/Texture.h"
#include "Renderer/GPUShared/ShadowData.h"
#include "render_resource_def.h"
#include <array>

namespace Render {
	class Camera;
	class Scene;
	class Light;
	struct rs_commandbuffer;
	class CascadedShadowPrivate;

	class CascadedShadow {
	public:
		static constexpr int MAX_CASCADE_LAYERS = 4;

	public:
		CascadedShadow();
		~CascadedShadow();

		void     setCascadedLayers(int layers);
		int      getCascadedLayers() const { return mCascadedLayers; }

		void     setCascadedLayerDistance(int layer, float dis);
		float    getCascadedLayerDistance(int layer) const;

		void     setCascadedInterpolateFactor(float x);
		float    getCascadedInterpolateFactor() const { return mBlendFactor; }

		void	 setCascadedProjectionCullSize(int layer, float x);
		float	 getCascadedProjectionCullSize(int layer);

		void     setTextureSize(uint32_t size);
		uint32_t getTextureSize() const { return mTextureSize; }

		void     setShadowEnable(bool enable);
		bool     isShadowEnable() const { return mIsEnable; }

		void	 setCameraHeight(float f);
		float	 getCameraHeight() const ;

		void draw(rs_commandbuffer* cmdBuffer, Camera* mainCamera, Scene* scene);

		TexturePtr getShadowTexture() const;
		const GPUShared::GPUDirLightShadowData& getShadowData() const { return mShadowData; }

		void prepareShadowResources();

	protected:
		void updateCascadeCameras(Camera* mainCamera, Light* mainDirLight);
		void updateDirlightShaderData(Light* light);
	private:
		CascadedShadowPrivate* mDp = nullptr;

		GPUShared::GPUDirLightShadowData mShadowData{};
		TexturePtr                       mShadowMap = nullptr;

		bool     mIsEnable = true;
		bool     mIsResourceDirty = true;
		uint32_t mTextureSize = 2048;
		int      mCascadedLayers = 3;
		float    mCascadedDistance[MAX_CASCADE_LAYERS] = { 10.0f, 35.0f, 150.0f, 300.0f };
		float	 mCascadedCullProjectionSize[MAX_CASCADE_LAYERS] = { 0.,5.,10.,20.};
		float    mBlendFactor = 0.1f;
	};
}

#endif // CASCADED_SHADOW_H_