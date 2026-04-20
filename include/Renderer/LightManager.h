#ifndef LIGHT_MANAGER_H_
#define LIGHT_MANAGER_H_
#include "common/Singleton.h"
#include "Renderer/Light.h"
#include <memory>
#include <map>
#include "renderer/GPUShared/SceneData.h"
#include "renderer/Texture.h"
namespace Render {
	class ComputeKernel;
	class LightManager{
	public:
		LightManager();
		~LightManager();
		//Do a copy internal
		int addLight(Light* light);
		void removeLight(int idx);
		Light* getLight(int idx);
		const Light* getLight(int idx) const;
		void update();


		void calculateIBLData(rs_commandbuffer* cmdbuf);
		const GPUShared::GPUSceneLightData& updateLightData();

	public:
		void setSkybox(TexturePtr skybox);
		void setSkyboxRotation(quat rotation);
		void setSkyboxExposure(float exposure);

	private:

		void calculatePrefilterEnv(rs_commandbuffer* cmd);
		void calculateBRDFLut(rs_commandbuffer* cmd);

		void calcMipMap(TexturePtr tex);

		struct LightData {
			Light* light;
			GPUShared::GPULightData data;
		};
		TexturePtr mSkybox;
		TexturePtr mBRDFLut;
		TexturePtr mPrefilterSkymap;
		std::map<int, LightData> lightMap;
		GPUShared::GPUSceneLightData mLightData;
		bool lightDataDirty = true;
		bool prefilterMapNeedGenerate = false;

		ComputeKernel* mipmapGenKernel		= nullptr;
		ComputeKernel* mipmapGenKernelCube = nullptr;
		ComputeKernel* irradianceMapKernel	= nullptr;
		ComputeKernel* brdfLUTKernel		= nullptr;
	};
}

#endif