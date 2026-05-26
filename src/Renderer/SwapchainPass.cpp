#include "Renderer/SwapchainPass.h"
#include <Renderer/MaterialTemplate.h>
#include "Renderer/RenderEntity.h"
#include <Renderer/RenderSystem.h>
#include "Renderer/RenderPassManager.h"
#include "Renderer/MaterialTemplateManager.h"
#include "Renderer/MaterialManager.h"
#include "common/ResourceSystem.h"
#include "Renderer/EnginePass.h"
namespace Render {

	class NormalEntity :public RenderEntity{
	public:
		NormalEntity() {
		}
		virtual void updateUniforms(rs_commandbuffer* cmd, MaterialPass* pass) {
			
		};

		virtual void updateEntityCommonDataImpl(Pass* pass) override{
			return;
		}

		Material* getMaterial() override{
			mMaterial = ResourceSystem::instance()->getResource<Material>(ResourceName::Material, Name("SwapChainMat"));
			if (!mMaterial) {
				auto matTempPtr = ResourceSystem::instance()->getResource<MaterialTemplate>(ResourceName::MaterialTemplate, Name("SwapChain"));
				ShaderStageInfo stageInfo{ {ShaderStage::Vertex,"../shader/Blit.vs"},{ShaderStage::Fragment,"../shader/Blit.ps"} };
				RenderState state{};
				state.depthTestEnable = false;
				VertexInputDescription vtxIA{};
				matTempPtr = MaterialTemplateManager::instance()->createMaterialTemplate(Name("SwapChain"), stageInfo, state, vtxIA);
				matTempPtr->createMaterialPass(PassName::SwapchainPass, {});
				mMaterial = MaterialManager::instance()->createMaterial<Material>(Name("SwapChainMat"), matTempPtr);
				mMaterial->addMaterialPassToRender(PassName::SwapchainPass);
			}
			return mMaterial.get();
		}
		MaterialPtr mMaterial = 0;
	};

	static PassDesc SwapchainPassDesc{
		.attachments = {
			PassAttachment{
				.fmt = RenderTextureFormat::SwapchainFormat,
				.loadOp = StorageOp::Clear,
				.storeOp = StorageOp::Cached
			}
		},
		.writeDepth = false
	};

	SwapchainPass::SwapchainPass():RenderPass(PassName::SwapchainPass, SwapchainPassDesc)
	{
		ClearColor SwapchainImgClrColor = {};
		SwapchainImgClrColor.rgba[0] = 0.f;
		SwapchainImgClrColor.rgba[1] = 0.f;
		SwapchainImgClrColor.rgba[2] = 0.f;
		SwapchainImgClrColor.rgba[3] = 1.f;
		this->setClearData({ SwapchainImgClrColor }, {});
		this->setRenderTarget(nullptr);
	}
	void SwapchainPass::init()
	{
		initBlitData();
	}
	SwapchainPass::~SwapchainPass()
	{
		auto RenderSys = RenderSystem::instance();
		RenderSys->destroyRsSampler(BlitRTSampler);
		delete BlitEntity;
	}

	void SwapchainPass::initBlitData()
	{
		BlitEntity = new NormalEntity();
		BlitEntity->getRenderInfo().idxCount = 6;

		auto BlitMatVarient = BlitEntity->getMaterial()->getMaterialPass(this->getPassName());
		BlitPass = BlitEntity->createPass(this->getPassName());

		auto RenderSys = RenderSystem::instance();
		BlitTarget = RenderSys->getBindingPos("u_prevRT", BlitMatVarient);
		BlitSampler = RenderSys->getBindingPos("u_sampler", BlitMatVarient);
		SamplerDesc samplerDesc{};
		BlitRTSampler = RenderSys->createSampler(samplerDesc);
	}

	void SwapchainPass::setBlitRT(rs_image* rt)
	{
		auto RenderSys = RenderSystem::instance();
		RenderSys->updateUniform(BlitTarget,0, rt, BlitPass);
	}

	void SwapchainPass::drawImpl(rs_commandbuffer* cmdbuffer)
	{
		auto RenderSys = RenderSystem::instance();
		RenderSys->setCurrentCamera(nullptr);
		auto nextSwapchainRt = RenderSys->getNextSwapchainRendertarget();
		RenderSys->cmdSetRendertarget(cmdbuffer, nextSwapchainRt);
		updateViewportAndScissor(cmdbuffer, nextSwapchainRt);
		RenderSys->drawIndexed(cmdbuffer, BlitEntity, BlitPass);
	}
	void SwapchainPass::collectRenderEntities(std::vector<RenderPack>& pack)
	{
		pack.push_back({ BlitEntity,BlitPass });
	}
}