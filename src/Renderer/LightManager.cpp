#include "Renderer/LightManager.h"
#include "Renderer/ComputeKernel.h"
#include "Renderer/SamplerResourceManager.h"
#include "Renderer/TextureResourceMgr.h"
#include "Renderer/GPUShared/IBLGenConfig.h"

static const int BRDF_LUT_MIPS = 6;
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
			auto linearSamplerPtr = SamplerResourceManager::instance()->getOrCreateSampler(linearSampler);
			mipmapGenKernel->setParameter("samplerSrc", linearSamplerPtr);
			mipmapGenKernelCube->setParameter("samplerSrc", SamplerResourceManager::instance()->getOrCreateSampler(linearSampler));
			irradianceMapKernel = new ComputeKernel("../shader/PreFilterIrradianceMap.cs", {});
			irradianceMapKernel->setParameter("EnvCubeMapSampler", linearSamplerPtr);
			brdfLUTKernel = new ComputeKernel("../shader/BRDFLut.cs", {});
		}
	}

	LightManager::~LightManager()
	{
		delete mipmapGenKernel;
		delete mipmapGenKernelCube;
		delete irradianceMapKernel;
		delete brdfLUTKernel;
	}

	int LightManager::addLight(Light* light)
	{
		static int LightIDX = 0;
		auto size = this->lightMap.size();
		if (size > RENDER_MAX_LIGHT_PER_SCENE) {
			return -1;
		}
		lightDataDirty = true;
		int idxCur = LightIDX++;
		lightMap.insert({ idxCur,LightData{.light = light} });
		return idxCur;
	}
	void LightManager::removeLight(int idx)
	{
		if (lightMap.find(idx) != lightMap.end()) {
			lightDataDirty = true;
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

	void LightManager::update()
	{
		
	}

	void LightManager::setSkybox(TexturePtr skybox)
	{
		mSkybox = skybox;
		if(skybox)
			prefilterMapNeedGenerate = true;
	}
	void LightManager::calculateIBLData(rs_commandbuffer* cmdbuf)
	{
		if (mSkybox == nullptr) {
			return;
		}
		auto rsys = RenderSystem::instance();
		if (!mBRDFLut) {
			auto brdfLutImg = rsys->createImage2D(nullptr, 0, ImageFormat::RGBA16_SFLOAT, 1024, 1024, 1, 1, 1);
			mBRDFLut = TextureResourceManager::instance()->createFromRsImage(Name("LightManager::BRDFLUT"),brdfLutImg);
			this->brdfLUTKernel->setParameter("OutBrdfLUT", mBRDFLut);
			brdfLUTKernel->dispatch(cmdbuf, 1024 / 8, 1024 / 8, 1);
		}
		if (!mPrefilterSkymap) {
			auto prefilterSkymap = rsys->createCubemap(0, 0, ImageFormat::RGBA16_SFLOAT, 1024, 1024, 1, 1, BRDF_LUT_MIPS);
			mPrefilterSkymap = TextureResourceManager::instance()->createFromRsImage(Name("LightManager::ENVMAP"), prefilterSkymap);
		}

		if (prefilterMapNeedGenerate) {
			prefilterMapNeedGenerate = false;
			GPUShared::PrefilterEnvMapCfg cfg{};

			irradianceMapKernel->setParameter("EnvCubeMap", this->mSkybox);

			for (float i = 0; i < BRDF_LUT_MIPS; i ++ ) {

				ImageViewKey key;
				key.setViewType(ImageType::VCube);
				key.setBaseMip(i).setMipCount(1).setLayerCount(6);
				irradianceMapKernel->setParameter("OutPrefilterEnvCubeMap", mPrefilterSkymap, key);

				cfg.curRoughness = i / (BRDF_LUT_MIPS - 1);
				irradianceMapKernel->setParameter("PrefilterCfg", cfg);
				irradianceMapKernel->dispatch(cmdbuf, 1024, 1024, 6);
			}

		}

	}
	const GPUShared::GPUSceneLightData& LightManager::updateLightData()
	{
		int idx = 0;

		for (auto& [lightIdx, lightData] : lightMap) {
			auto& light = lightData.light;
			bool needUpdate = lightDataDirty;
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
		lightDataDirty = false;
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
