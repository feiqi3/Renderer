#ifndef LIGHT_MANAGER_H_
#define LIGHT_MANAGER_H_
#include "common/Singleton.h"
#include "Renderer/Light.h"
#include <memory>
#include <map>
#include "renderer/GPUShared/SceneData.h"
namespace Render {
	class LightManager{
	public:
		LightManager();
		//Do a copy internal
		int addLight(Light* light);
		void removeLight(int idx);
		Light* getLight(int idx);
		const Light* getLight(int idx) const;
		const GPUShared::GPUSceneLightData& updateLightData();
	private:
		struct LightData {
			Light* light;
			GPUShared::GPULightData data;
		};

		std::map<int, LightData> lightMap;
		GPUShared::GPUSceneLightData mLightData;
		bool dirty = true;
	};
}

#endif