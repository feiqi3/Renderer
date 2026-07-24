#ifndef SHADOW_MANAGER_H_
#define SHADOW_MANAGER_H_

#include "Renderer/Texture.h"
#include "render_resource_def.h"
#include "Renderer/SamplerResourceManager.h"
#include "Renderer/GPUShared/ShadowData.h"
#include "Renderer/Shadows/CascadedShadow.h"
#include <memory>

namespace Render {
	class Camera;
	class Scene;
	class Light;
	struct rs_commandbuffer;

	class ShadowManager {
	public:
		enum class ShadowTechnique {
			Normal,
			PCF,
		};

	public:
		ShadowManager();
		~ShadowManager();

		void setShadowEnable(bool isEnable);
		bool getShadowEnable() const;

		void setShadowTechnique(ShadowTechnique tech);
		ShadowTechnique getShadowTechnique() const;

		void setShadowTexSize(uint32_t size);
		void setShadowTexFormat(RenderTextureFormat fmt);
		void setShadowCameraHeight(float h);

		void setCascadedLayers(int layers);
		int  getCascadedLayers() const;

		void  setCascadedLayerDistance(int layer, float dis);
		float getCascadedLayerDistance(int layer) const;

		void  setCascadedInterpolateFactor(float x);
		float getCascadedInterpolateFactor() const;

		void setPointLightMaxCount(uint32_t count);

		void drawShadow(rs_commandbuffer* cmdBuffer, Camera* currentCamera, Scene* scene);

		rs_drawdata* getShadowDrawData() const;
		TexturePtr                    getDirShadowTexture() const;
		SamplerPtr                    getShadowSampler() const;
		GPUShared::GPUSceneShadowData getSceneShadowData() const;

		CascadedShadow* getCascadedShadow() const;


	private:
		bool            mIsShadowEnable = true;
		ShadowTechnique mTechnique = ShadowTechnique::PCF;

		class ShadowManagerPrivate* mDp = nullptr;
	};
}

#endif // SHADOW_MANAGER_H_