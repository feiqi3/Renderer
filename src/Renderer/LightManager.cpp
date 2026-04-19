#include "Renderer/LightManager.h"
#include "Renderer/ComputeKernel.h"
#include "Renderer/SamplerResourceManager.h"
namespace Render {
	LightManager::LightManager()
	{
		{

			//1. mipmap gen kernel
			MacroPairs pairs2D{
				{"USE_SAMPLER",{}},
				{"USE_TARGET_IMAGE_TYPE","0"}
			};
			mipmapGenKernel = new ComputeKernel("../shader/mipmapGen.cs", pairs2D);
			MacroPairs pairs2DArray{
			{"USE_SAMPLER",{}},
			{"USE_TARGET_IMAGE_TYPE","1"}
			};
			mipmapGenKernelCube = new ComputeKernel("../shader/mipmapGen.cs", pairs2DArray);
			SamplerDesc linearSampler{};
			linearSampler.addressU = AddressMode::ClampToEdge;
			linearSampler.addressV = AddressMode::ClampToEdge;
			mipmapGenKernel->setParameter("samplerSrc", SamplerResourceManager::instance()->getOrCreateSampler(linearSampler));
			mipmapGenKernelCube->setParameter("samplerSrc", SamplerResourceManager::instance()->getOrCreateSampler(linearSampler));

		}
	}

	LightManager::~LightManager()
	{
		delete mipmapGenKernel;
		delete mipmapGenKernelCube;
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
	void LightManager::calculateIBLData(TexturePtr irradianceMap, TexturePtr brdfLUT)
	{
	}
	const GPUShared::GPUSceneLightData& LightManager::updateLightData()
	{
		int idx = 0;

		for (auto& [lightIdx, lightData] : lightMap) {
			auto& light = lightData.light;
			bool needUpdate = dirty;
			if (light->isDirty()) {
				lightData.data = light->toGPUData();
				needUpdate = true;
			}
			if (needUpdate) {
				mLightData.lights[idx] = lightData.data;
				light->setDirty(false);
			}
			idx = idx + 1;
		}
		mLightData.sceneLightInfo.x = idx;
		dirty = false;
		return mLightData;
	}
	void LightManager::calcMipMap(TexturePtr tex)
	{
		//1. Calculate MipMap Level Count
		uint32_t mipLevelCount = 1 + (uint32_t)std::floor(std::log2(std::max(tex->getRsImage()->height, tex->getRsImage()->width)));
		mipLevelCount = std::min(mipLevelCount, (uint32_t)tex->getRsImage()->mipLevels);
		//Bind parameter
		struct MipMapGenCfg {
			uint32_t mipsCount;
			uint32_t numWorkGroups;// use dispatch z as total slice count
			uint32_t workGroupOffset;
			uint32_t imageSizeX;
			uint32_t imageSizeY;
			float invImageSizeX;
			float invImageSizeY;
		}cfg;
		cfg.mipsCount = mipLevelCount;
		cfg.numWorkGroups = tex->getRsImage()->arrayLayers;
		cfg.workGroupOffset = 0;
		cfg.imageSizeX = tex->getRsImage()->width;
		cfg.imageSizeY = tex->getRsImage()->height;
		cfg.invImageSizeX = 1.0f / cfg.imageSizeX;
		cfg.invImageSizeY = 1.0f / cfg.imageSizeY;

		if (cfg.numWorkGroups > 1) {
			//1. use cube mipmap gen kernel
			mipmapGenKernelCube->setParameter("MipMapGen",&cfg,sizeof(cfg));
			ImageViewKey viewKey{};
			viewKey.setAspect(ViewAspect::Color)
				.setViewType(ImageType::V2D_Array)
				.setBaseMip(0)
				.setMipCount(mipLevelCount)
				.setBaseLayer(0)
				.setLayerCount(tex->getRsImage()->arrayLayers);
		}

	}
}
