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
		this->createPass(PassName::MainCameraPass);
		this->getMaterial()->setRenderOrder(RenderOrder::SkyBox);
		GPUShared::SkyBoxData sdata{
		.rotateQuat = vec4(0,0,0,1),
		.colorexposureTuning = vec4(1.,1.,1.,1.)
		};
		this->getMaterial()->bindParameter("skyboxData", &sdata, sizeof(GPUShared::SkyBoxData));
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
			skyBoxRenderState.depthWriteEnable = true;
			skyBoxRenderState.depthTestEnable = true;
			skyBoxRenderState.depthCompareOp = CompareOp::LessOrEqual;
			VertexInputDescription iaDesc{};
			//1. Try create mat template
			matTplt = MaterialTemplateManager::instance()->createMaterialTemplate(Name("SkyBox"), {
				{ShaderStage::Vertex, "../shader/SkyBox.vs"},{ShaderStage::Fragment,"../shader/SkyBox.ps"}
				}, skyBoxRenderState, iaDesc);
			matTplt->createMaterialPass(RenderSystem::instance()->getRenderPass(PassName::MainCameraPass));
		}
		MaterialPtr matSkyBox = MaterialManager::instance()->getMaterial(Name("SkyBoxMaterial"));
		if (!matSkyBox)
		{
			matSkyBox = MaterialManager::instance()->createMaterial<Material>(Name("SkyBoxMaterial"), matTplt);
			matSkyBox->addMaterialPassToRender(PassName::MainCameraPass);
		}
		return matSkyBox;
	}

	void SkyBoxRenderEntity::setSkyboxCubemap(TexturePtr texture, SamplerPtr sampler)
	{
		this->getMaterial()->bindParameter("SkyboxCubeMap", texture);
		this->getMaterial()->bindParameter("cubemapSampler", sampler);
	}

	void SkyBoxRenderEntity::setGPUData(const GPUShared::SkyBoxData& data) {
		this->getMaterial()->bindParameter("skyboxData", &data,sizeof(GPUShared::SkyBoxData));
	}


}
