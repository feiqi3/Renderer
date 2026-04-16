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
		void calculateIBLData(TexturePtr irradianceMap,TexturePtr brdfLUT);
		const GPUShared::GPUSceneLightData& updateLightData();
	private:

		void calcMipMap(TexturePtr tex);

		struct LightData {
			Light* light;
			GPUShared::GPULightData data;
		};

		std::map<int, LightData> lightMap;
		GPUShared::GPUSceneLightData mLightData;
		bool dirty = true;
		ComputeKernel* mipmapGenKernel		= nullptr;
		ComputeKernel* mipmapGenKernelCube = nullptr;
		ComputeKernel* irradianceMapKernel	= nullptr;
		ComputeKernel* brdfLUTKernel		= nullptr;
	};
}

#endif