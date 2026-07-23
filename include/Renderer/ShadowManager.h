#ifndef SHADOW_MANAGER_H_
#define SHADOW_MANAGER_H_
#include "Renderer/Texture.h"
#include "render_resource_def.h"
#include "Renderer/SamplerResourceManager.h"
#include "Renderer/GPUShared/ShadowData.h"
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
		ShadowManager ();
		~ShadowManager();
		void setPointLightMaxCount(uint32_t count);
		void setShadowTexSize(uint32_t size);
		void setShadowTexFormat(RenderTextureFormat fmt);
		void setShadowCameraHeight(float h);
		void setDirLightShadowFarZ(float f);

		bool getShadowEnable	()const;
		void setShadowEnable	(bool isEnable);

		ShadowTechnique getShadowTechnique()const;
		void			setShadowTechnique(ShadowTechnique tech);

		//Update some nessary data light camera or render texture
		//Before cameramanager update and before scene update.
		//void update(Scene* scene);
		void drawShadow(rs_commandbuffer* cmdBuffer,Camera* currentCamera,Scene* scene);
		rs_drawdata* getShadowDrawData()const;
		TexturePtr	getDirShadowTexture()const;
		SamplerPtr	getShadowSampler()const;
		GPUShared::GPUSceneShadowData getSceneShadowData()const;
	protected:
		void setDirLightCamera(rs_commandbuffer* cmdBuffer,Light* light, Camera* currentCamera);
		void processShadowDrawInfo(Scene* scene);
		void prepareDirShadowResource();
	private:
		struct  {
			uint32_t shadowRTSize = 2048;//x, y
			RenderTextureFormat shadowRTFormat = RenderTextureFormat::D32;
			int pointLightShadowNum = 0;
			float dirLightCameraHeight = 100.f;
			float dirLightShadowFarZ = 20.f;
		}ShadowConfig;
		bool isDirShadowRTDirty = true;
		bool isDirShadowConfigDirty = true;
		bool isPointShadowConfigDirty = true;

		bool isShadowEnable = true;
		ShadowTechnique mTechnique;

		class ShadowManagerPrivate* mDp;;

		TexturePtr mDirLightShadowMap;
	};
};

#endif