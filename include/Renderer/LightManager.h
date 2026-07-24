#ifndef LIGHT_MANAGER_H_
#define LIGHT_MANAGER_H_
#include "common/Singleton.h"
#include "Renderer/Light.h"
#include <memory>
#include <map>
#include "renderer/GPUShared/SceneData.h"
#include "renderer/Texture.h"
#include "renderer/SamplerResourceManager.h"
namespace Render {
	class ComputeKernel;
	class LightManager{
	public:

		struct LightData {
			Light* light;
			GPUShared::GPULightData data;
		};

	public:
		LightManager();
		~LightManager();
		//Do a copy internal
		int addLight(Light* light);
		void removeLight(int idx);
		Light* getLight(int idx);
		const Light* getLight(int idx) const;
		void update();
		void setShadowLightNum(uint32_t lightNum);
		inline const std::vector<Light*>& lightsToCastShadow()const {
			return mShadowLights;
		}

		inline Light* getMainDirLight()const {
			return mMainDirLight;
		}

		TexturePtr getBRDFLut();
		TexturePtr getPrefilterEnvMap();
		SamplerPtr getIBLSampler();
		void calculateIBLData(rs_commandbuffer* cmdbuf);
		const GPUShared::GPUSceneLightData& updateLightData();
		const std::map<int, LightData>& getLightMap();
	public:
		void setSkybox(TexturePtr skybox);
		//void setSkyboxRotation(quat rotation);
		//void setSkyboxExposure(float exposure);

	private:


		void calcMipMap(TexturePtr tex);


		TexturePtr mSkybox;
		TexturePtr mBRDFLut;
		TexturePtr mPrefilterSkymap;
		SamplerPtr mSampler;
		std::map<int, LightData> mLightMap;
		GPUShared::GPUSceneLightData mLightData;

		Light* mMainDirLight = nullptr;
		std::vector<Light*> mShadowLights;
		int		mShadowLightNum = -1;

		bool lightDataDirty = true;
		bool prefilterMapNeedGenerate = false;
		bool brdfLutNeedGenerate = true;

		ComputeKernel* mipmapGenKernel		= nullptr;
		ComputeKernel* mipmapGenKernelCube = nullptr;
		ComputeKernel* irradianceMapKernel	= nullptr;
		ComputeKernel* brdfLUTKernel		= nullptr;
	};
}

#endif