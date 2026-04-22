#include "Renderer/LightManager.h"
#include "Renderer/ComputeKernel.h"
#include "Renderer/SamplerResourceManager.h"
#include "Renderer/TextureResourceMgr.h"
#include "Renderer/GPUShared/IBLGenConfig.h"
#include "common/ResourceSystem.h"
namespace Render {
	static int MAX_MIPS_TO_GEN = 5;
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

			auto brdfLutImg = RenderSystem::instance()->createImage2D(nullptr, 0, ImageFormat::RGBA16_SFLOAT, 1024, 1024, 1, 1, 1);
			mBRDFLut = TextureResourceManager::instance()->createFromRsImage(Name("LightManager::BRDFLUT"), brdfLutImg);

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
		if(skybox)
		{
			auto rsImage = skybox->getRsImage();
			if (mSkybox == nullptr || rsImage->width != mSkybox->getRsImage()->width ||
				rsImage->height != mSkybox->getRsImage()->height
				) {
				auto width = rsImage->width;
				auto height = rsImage->height;
				int mipsToGen = 1;
				auto maxEdge = std::max(width, height);
				while (maxEdge /= 2) {
					mipsToGen++;
				}
				mipsToGen = std::min(mipsToGen, MAX_MIPS_TO_GEN);
				auto rsImage = RenderSystem::instance()->createCubemap(0, 0, ImageFormat::RGBA16_SFLOAT, width, height, 1, 1, mipsToGen);
				if (!mPrefilterSkymap) {
					mPrefilterSkymap = TextureResourceManager::instance()->createEmpty(Name("LightManager::PrefilterENV"));
				}
				mPrefilterSkymap->setRsImage(rsImage);
			}
			mSkybox = skybox;
			prefilterMapNeedGenerate = true;
		}
	}
	TexturePtr LightManager::getBRDFLut()
	{
		return mBRDFLut;
	}
	TexturePtr LightManager::getPrefilterEnvMap()
	{
		return mPrefilterSkymap;
	}
	SamplerPtr LightManager::getIBLSampler()
	{
		if (!mSampler) {
			SamplerDesc desc{};
			desc.addressU = AddressMode::ClampToEdge;
			desc.addressV = AddressMode::ClampToEdge;
			desc.addressW = AddressMode::ClampToEdge;
			desc.mipmapMode = MipMapMode::Cubic;
			mSampler = SamplerResourceManager::instance()->getOrCreateSampler(desc);
		}
		return mSampler;
	}
	void LightManager::calculateIBLData(rs_commandbuffer* cmdbuf)
	{
		if (mSkybox == nullptr) {
			return;
		}
		auto rsys = RenderSystem::instance();
		if (brdfLutNeedGenerate) {
			brdfLutNeedGenerate = false;
			this->brdfLUTKernel->setParameter("OutBrdfLUT", mBRDFLut);
			brdfLUTKernel->dispatch(cmdbuf, 1024 / 8, 1024 / 8, 1);
		}

		if (prefilterMapNeedGenerate) {
			this->lightDataDirty = true;
			prefilterMapNeedGenerate = false;
			GPUShared::PrefilterEnvMapCfg cfg{};

			irradianceMapKernel->setParameter("EnvCubeMap", this->mSkybox);
			auto rsImagePrefilter = mPrefilterSkymap->getRsImage();
			auto mipsOfEnvMap = rsImagePrefilter->mipLevels;
			int longEdge = std::max(rsImagePrefilter->width, rsImagePrefilter->height);
			int imageSize = longEdge;
			for (float i = 0; i < mipsOfEnvMap; i ++ ) {

				ImageViewKey key;
				key.setViewType(ImageType::VCube);
				key.setBaseMip(i).setMipCount(1).setLayerCount(6);
				irradianceMapKernel->setParameter("OutPrefilterEnvCubeMap", mPrefilterSkymap, key);

				cfg.curRoughness = i / (mipsOfEnvMap - 1);
				irradianceMapKernel->setParameter("PrefilterCfg", cfg);
				irradianceMapKernel->dispatch(cmdbuf, imageSize / 8, imageSize / 8, 6);
				imageSize /= 2;

			}

		}

	}
	const GPUShared::GPUSceneLightData& LightManager::updateLightData()
	{
		int idx = 0;
		vec4 sceneLightData;
		if (this->mPrefilterSkymap) {
			sceneLightData.x = mPrefilterSkymap->getRsImage()->mipLevels;
		}
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
		if (this->mPrefilterSkymap) {
			mLightData.IBLControl.x = mPrefilterSkymap->getRsImage()->mipLevels;
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
