#include "Renderer/SkyboxRenderEntity.h"
#include "Renderer/MaterialManager.h"
#include "Renderer/MaterialTemplateManager.h"
#include "Renderer/EnginePass.h"
#include "Renderer/RenderQueue.h"
#include "Renderer/GPUShared/SkyBoxData.h"
namespace Render {
	SkyBoxRenderEntity::SkyBoxRenderEntity()
	{
		//1. Set RenderInfo
		auto& renderInfo = getRenderInfo();
		renderInfo.idxCount = 3;
		this->mSkyboxMaterial = getSkyBoxMaterial();
		this->getMaterial()->setRenderOrder(RenderOrder::SkyBox);
		//2. set Render Mask
		mRenderMask = RenderMask::SkyBox;
	}

	Render::Material* SkyBoxRenderEntity::getMaterial()
	{
		return mSkyboxMaterial.get();
	}

	Render::MaterialPtr SkyBoxRenderEntity::getSkyBoxMaterial()
	{
		MaterialTemplatePtr matTplt = MaterialTemplateManager::instance()->getMaterialTemplate(Name("SkyBox"));
		if (matTplt == nullptr) {
			RenderState skyBoxRenderState{};
			skyBoxRenderState.depthWriteEnable = false;
			skyBoxRenderState.depthTestEnable = true;
			skyBoxRenderState.depthCompareOp = CompareOp::LessOrEqual;
			VertexInputDescription iaDesc{};
			//1. Try create mat template
			matTplt = MaterialTemplateManager::instance()->createMaterialTemplate(Name("SkyBox"), {
				{ShaderStage::Vertex, "../shader/SkyBox.vs"},{ShaderStage::Fragment,"../shader/SkyBox.ps"}
				}, skyBoxRenderState, iaDesc);
			matTplt->createMaterialPass(PassName::SkyboxPass);
		}
		MaterialPtr matSkyBox = MaterialManager::instance()->getMaterial(Name("SkyBoxMaterial"));
		if (!matSkyBox)
		{
			matSkyBox = MaterialManager::instance()->createMaterial<Material>(Name("SkyBoxMaterial"), matTplt);
		}
		matSkyBox->setRenderMask(RenderMask::SkyBox);
		return matSkyBox;
	}

	void SkyBoxRenderEntity::setSkyboxCubemap(TexturePtr texture, SamplerPtr sampler)
	{
		this->getMaterial()->bindParameter("SkyboxCubeMap", texture);
		this->getMaterial()->bindParameter("cubemapSampler", sampler);
		auto textureFmt = texture->getRsImage()->format;
		mSkyboxTexture = texture;
		mSkyboxSampler = sampler;
		bool isHDR = (textureFmt == ImageFormat::RGBA16_SFLOAT) ||
			(textureFmt == ImageFormat::RGBA32_SFLOAT) ||
			(textureFmt == ImageFormat::R11G11B10_UFLOAT);
		GPUShared::SkyBoxData sdata{
.rotateQuat = vec4(0,0,0,1),
.colorexposureTuning = vec4(1.,1.,1.,1.),
.isHDR = vec4(isHDR ? 1 : -1,0,0,0)
		};
		this->getMaterial()->bindParameter("skyboxData", &sdata, sizeof(GPUShared::SkyBoxData));

	}

	void SkyBoxRenderEntity::setGPUData(const GPUShared::SkyBoxData& data) {
		auto textureFmt = mSkyboxTexture->getRsImage()->format;
		bool isHDR = (textureFmt == ImageFormat::RGBA16_SFLOAT) ||
			(textureFmt == ImageFormat::RGBA32_SFLOAT) ||
			(textureFmt == ImageFormat::R11G11B10_UFLOAT);
		auto dataCopy = data;
		dataCopy.isHDR.x = isHDR ? 1. : -1.;
		this->getMaterial()->bindParameter("skyboxData", &dataCopy,sizeof(GPUShared::SkyBoxData));
	}


	Render::AxisAlignedBoundingBox SkyBoxRenderEntity::getWorldBounding()
	{
		return AxisAlignedBoundingBox();
	}

}
