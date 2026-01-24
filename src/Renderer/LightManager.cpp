#include "Renderer/LightManager.h"
namespace Render {
	LightManager::LightManager()
	{
	}
	int LightManager::addLight(Light* light)
	{
		static int LightIDX = 0;
		auto size = this->lightMap.size();
		if (size > RENDER_MAX_LIGHT_PER_SCENE) {
			return -1;
		}
		dirty = true;
		int idxCur = LightIDX++;
		lightMap.insert({ idxCur,LightData{.light = light} });
		return idxCur;
	}
	void LightManager::removeLight(int idx)
	{
		if (lightMap.find(idx) != lightMap.end()) {
			dirty = true;
			lightMap.erase(idx);
		}
	}
	Light* LightManager::getLight(int idx)
	{
		auto it = lightMap.find(idx);
		if (it == lightMap.end()) {
			return nullptr;
		}
		return it->second.light;
	}
	const Light* LightManager::getLight(int idx) const
	{
		auto it = lightMap.find(idx);
		if (it == lightMap.end()) {
			return nullptr;
		}
		return it->second.light;
	}
	const GPUShared::GPUSceneLightData& LightManager::updateLightData()
	{
		int idx = 0;

		for (auto& [lightIdx, lightData] : lightMap) {
			auto light = lightData.light;
			bool needUpdate = dirty;
			if (light->isDirty()) {
				lightData.data = light->toGPUData();
				needUpdate = true;
			}
			if (needUpdate) {
				mLightData.lights[idx] = lightData.data;
			}
			idx = idx + 1;
		}
		mLightData.sceneLightInfo.x = idx;
		dirty = false;
		return mLightData;
	}
}
