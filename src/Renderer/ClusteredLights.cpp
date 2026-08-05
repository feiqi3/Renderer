#include "Renderer/ClusteredLights.h"
#include "Renderer/ComputeKernel.h"
#include "Renderer/Texture.h"
#include "Renderer/TextureResourceMgr.h"
#include "Renderer/SamplerResourceManager.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/RenderDebuger.h"
#include "Renderer/LightManager.h"
#include "Renderer/Camera.h"
#include "function/Scene.h"
#include "renderer/GPUShared/SceneLightsCullData.h"
#include <algorithm>

namespace Render {

	class HizClusteredLightPrivate {
	public:
		ComputeKernel* mFroxelBuilderKernel = nullptr; // Pass 0: FroxelData
		ComputeKernel* mLightCullHiZKernel = nullptr; // Pass 1: Hi-Z 2D
		ComputeKernel* mClusterBuilderKernel = nullptr; // Pass 2: 3D Froxel

		rs_buffer* mFroxelConfigBuffer = nullptr; // UBO: FroxelConfigData
		rs_buffer* mFroxelListBuffer = nullptr; // SSBO: FroxelInfo[]
		rs_buffer* mPassHiZLightDataBuffer = nullptr; // SSBO: LightCulledDataList (Pass 1 output)
		rs_buffer* mFroxelLightDataBuffer = nullptr; // SSBO: FroxelLightDataList[] (Pass 2 output)

		uint32_t mPassHiZCapacity = 0;

		SamplerPtr mPointSampler;                       

		uint32_t mTileXMax = 16;
		uint32_t mTileYMax = 9;
		uint32_t mTileZMax = 24;
		float    mSpecialNear = 1.0f;

		bool mFroxelGridDirty = true;
	};

	HizClusteredLight::HizClusteredLight()
	{
		mDp = std::make_unique<HizClusteredLightPrivate>();

		mDp->mFroxelBuilderKernel = new ComputeKernel("../shader/FroxelBuilder.cs", {});
		mDp->mLightCullHiZKernel = new ComputeKernel("../shader/LightCullHiZ.cs", {});
		mDp->mClusterBuilderKernel = new ComputeKernel("../shader/ClusterLightsBuilder.cs", {});

		SamplerDesc sampDesc{};
		sampDesc.addressU = AddressMode::ClampToEdge;
		sampDesc.addressV = AddressMode::ClampToEdge;
		sampDesc.minFilter = Filter::Nearest;
		sampDesc.magFilter = Filter::Nearest;
		sampDesc.mipmapMode = Filter::Nearest;
		mDp->mPointSampler = SamplerResourceManager::instance()->getOrCreateSampler(sampDesc);

		mDp->mLightCullHiZKernel->setParameter("PointSampler", mDp->mPointSampler);

		mClusterInfo.MaxTileX = mDp->mTileXMax;
		mClusterInfo.MaxTileY = mDp->mTileYMax;
		mClusterInfo.MaxTileZ = mDp->mTileZMax;
		mClusterInfo.specialNear = mDp->mSpecialNear;
	}

	HizClusteredLight::~HizClusteredLight()
	{
		delete mDp->mFroxelBuilderKernel;
		delete mDp->mLightCullHiZKernel;
		delete mDp->mClusterBuilderKernel;

		if (mDp->mFroxelConfigBuffer)     RenderSystem::instance()->destroyBuffer(mDp->mFroxelConfigBuffer);
		if (mDp->mFroxelListBuffer)       RenderSystem::instance()->destroyBuffer(mDp->mFroxelListBuffer);
		if (mDp->mPassHiZLightDataBuffer) RenderSystem::instance()->destroyBuffer(mDp->mPassHiZLightDataBuffer);
		if (mDp->mFroxelLightDataBuffer)  RenderSystem::instance()->destroyBuffer(mDp->mFroxelLightDataBuffer);
	}

	void HizClusteredLight::setHiZTexture(const TexturePtr& hizTex)
	{
		mHizTex = hizTex;
	}

	void HizClusteredLight::setLightListBuffer(rs_buffer* lightBuffer, uint32_t lightCount)
	{
		mExternalLightBuffer = lightBuffer;
		mExternalLightCount = lightCount;
	}

	rs_buffer* HizClusteredLight::getFroxelLightDataBuffer() const
	{
		return mDp->mFroxelLightDataBuffer;
	}

	const Render::GPUShared::ClusterInfo& HizClusteredLight::getClusterInfo() const
	{
		return mClusterInfo;
	}

	struct FroxelConfigData {
		mat4 viewMat;
		mat4 ProjMat;
		mat4 invProjMat;
		mat4 invViewProjMat;
		vec3 camPosition;
		float screenSizeX;
		float screenSizeY;
		float specialNear;
		float camNear;
		float camZFar;
		int tileXMax;
		int tileYMax;
		int tileZMax;
	};

	void HizClusteredLight::draw(rs_commandbuffer* cmdBuffer, Camera* cam, Scene* scene)
	{
		if (!scene || !cam || !mHizTex || !mExternalLightBuffer || mExternalLightCount == 0) return;

		RenderMarker marker(cmdBuffer, "Clustered Light HiZ Culling", 0.2f, 0.6f, 0.8f, 1.0f);

		mClusterInfo.far = cam->getFar();
		mClusterInfo.near = cam->getNear();
		mClusterInfo.screenXY.x = mHizTex->getWidth() * 2;
		mClusterInfo.screenXY.y = mHizTex->getHeight() * 2;
		uint32_t totalLightCount = mExternalLightCount;
		uint32_t totalFroxels = mDp->mTileXMax * mDp->mTileYMax * mDp->mTileZMax;

		constexpr uint32_t kHeaderByteSize = sizeof(int) * 4;

		// =====================================================================
		// =====================================================================

		if (!mDp->mFroxelConfigBuffer) {
			BufferDesc desc{};
			desc.bufUsage = BufferType_Storage;
			desc.byteSize = sizeof(FroxelConfigData);
			mDp->mFroxelConfigBuffer = RenderSystem::instance()->createBuffer(nullptr, desc.byteSize, desc);
		}

		if (!mDp->mFroxelListBuffer) {
			BufferDesc desc{};
			desc.bufUsage = BufferType_Storage;
			desc.byteSize = totalFroxels * sizeof(GPUShared::FroxelInfo);
			mDp->mFroxelListBuffer = RenderSystem::instance()->createBuffer(nullptr, desc.byteSize, desc);
		}

		if (!mDp->mPassHiZLightDataBuffer || mDp->mPassHiZCapacity < totalLightCount) {
			if (mDp->mPassHiZLightDataBuffer) {
				RenderSystem::instance()->destroyBuffer(mDp->mPassHiZLightDataBuffer);
			}
			mDp->mPassHiZCapacity = std::max(totalLightCount, 64u);
			uint32_t allocSize = kHeaderByteSize + mDp->mPassHiZCapacity * sizeof(GPUShared::LightCullData);

			BufferDesc desc{};
			desc.bufUsage = BufferType_Storage;
			desc.byteSize = allocSize;
			mDp->mPassHiZLightDataBuffer = RenderSystem::instance()->createBuffer(nullptr, desc.byteSize, desc);
		}

		uint32_t pass2BufferSize = totalFroxels * sizeof(GPUShared::FroxelLightDataList);
		if (!mDp->mFroxelLightDataBuffer) {
			BufferDesc desc{};
			desc.bufUsage = BufferType_Storage;
			desc.byteSize = pass2BufferSize;
			mDp->mFroxelLightDataBuffer = RenderSystem::instance()->createBuffer(nullptr, desc.byteSize, desc);
		}

		// =====================================================================
		// =====================================================================
		struct CulledDataHeader {
			int lightCount;
			float padding0;
			float padding1;
			int padding2;
		} culledHeader{ 0, 0.0f, 0.0f, 0 };

		RenderSystem::instance()->updateBufferData(mDp->mPassHiZLightDataBuffer, &culledHeader, kHeaderByteSize, 0);

		// =====================================================================
		// =====================================================================
		//if (mDp->mFroxelGridDirty) {
			mDp->mFroxelBuilderKernel->setParameter("SSBO_froxelConfig", mDp->mFroxelConfigBuffer);
			mDp->mFroxelBuilderKernel->setParameter("SSBO_froxelList", mDp->mFroxelListBuffer);

			mDp->mFroxelBuilderKernel->dispatch(cmdBuffer, (mDp->mTileXMax + 7) / 8, (mDp->mTileYMax + 7) / 8, mDp->mTileZMax);
			mDp->mFroxelGridDirty = false;
			RenderSystem::instance()->cmdFlushUAVBuffer(cmdBuffer, mDp->mFroxelListBuffer);
		//}


		// =====================================================================
		// =====================================================================


		FroxelConfigData froxelConfig{};
		froxelConfig.camNear = cam->getNear();
		froxelConfig.camZFar = cam->getFar();
		froxelConfig.camPosition = cam->getPosition();
		froxelConfig.invViewProjMat = inverse(cam->getProjectionMatrix() * cam->getViewMatrix());
		froxelConfig.invProjMat = inverse(cam->getProjectionMatrix());
		froxelConfig.ProjMat = cam->getProjectionMatrix();
		froxelConfig.viewMat = cam->getViewMatrix();
		froxelConfig.screenSizeX = mHizTex->getWidth() * 2;
		froxelConfig.screenSizeY = mHizTex->getHeight() * 2;
		froxelConfig.specialNear = 4; // This is a magic
		froxelConfig.tileXMax = mDp->mTileXMax;
		froxelConfig.tileYMax = mDp->mTileYMax;
		froxelConfig.tileZMax = mDp->mTileZMax;
		RenderSystem::instance()->updateBufferData(mDp->mFroxelConfigBuffer, &froxelConfig, sizeof(froxelConfig), 0);

		// =====================================================================
		// =====================================================================
		mDp->mLightCullHiZKernel->setParameter("HizTex", mHizTex);
		struct {
			int lightCount;
			int pad0;
			int pad1;
			int pad2;
		}lightInfo{};
		lightInfo.lightCount = mExternalLightCount;
		mDp->mLightCullHiZKernel->setParameter("LightInfo", &lightInfo, sizeof(lightInfo));
		mDp->mLightCullHiZKernel->setParameter("SSBO_lightList", mExternalLightBuffer);
		mDp->mLightCullHiZKernel->setParameter("SSBO_passHiZlightDataList", mDp->mPassHiZLightDataBuffer);
		mDp->mLightCullHiZKernel->setParameter("SSBO_froxelConfig", mDp->mFroxelConfigBuffer);

		mDp->mLightCullHiZKernel->dispatch(cmdBuffer, (totalLightCount + 63) / 64, 1, 1);

		// =====================================================================
		// =====================================================================
		mDp->mClusterBuilderKernel->setParameter("SSBO_passHiZlightDataList", mDp->mPassHiZLightDataBuffer);
		mDp->mClusterBuilderKernel->setParameter("SSBO_froxelLightData", mDp->mFroxelLightDataBuffer);
		mDp->mClusterBuilderKernel->setParameter("SSBO_froxelList", mDp->mFroxelListBuffer);
		mDp->mClusterBuilderKernel->setParameter("SSBO_froxelConfig", mDp->mFroxelConfigBuffer);

		RenderSystem::instance()->cmdFlushUAVBuffer(cmdBuffer, mDp->mPassHiZLightDataBuffer);

		mDp->mClusterBuilderKernel->dispatch(
			cmdBuffer,
			(mDp->mTileXMax + 7) / 8,
			(mDp->mTileYMax + 7) / 8,
			mDp->mTileZMax
		);
		RenderSystem::instance()->cmdFlushUAVBuffer(cmdBuffer, mDp->mFroxelLightDataBuffer);
	}

} // namespace Render