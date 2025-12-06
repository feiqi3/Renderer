#include "Renderer/SwapchainPass.h"
#include <Renderer/MaterialTemplate.h>
#include "Renderer/RenderEntity.h"
#include <Renderer/RenderSystem.h>
#include "Renderer/RenderPassManager.h"
#include "Renderer/MaterialTemplateManager.h"
namespace Render {

	class NormalEntity :public RenderEntity{
	public:
		NormalEntity() {
		}

		MaterialTemplate* getMaterialTemplate() override{
			static bool isFirstInit = true;
			if (isFirstInit) {
				isFirstInit = false;
				ShaderStageInfo stageInfo{ {ShaderStage::Vertex,"../shader/Blit.vs"},{ShaderStage::Fragment,"../shader/Blit.ps"} };
				RenderState state{};
				state.depthTestEnable = false;
				VertexInputDescription vtxIA{};
				this->mMaterial = MaterialTemplateManager::instance()->createMaterialTemplate(Name("SwapChain"), stageInfo, state, vtxIA);
			}
			else if(mMaterial == nullptr){
				mMaterial = MaterialTemplateManager::instance()->getMaterialTemplate(Name("SwapChain"));
			}
			return mMaterial;
		}
	private: 
		MaterialTemplate* mMaterial = 0;
	};

	static PassDesc SwapchainPassDesc{
		.attachments = {
			PassAttachment{
				.loadOp = StorageOp::Clear,
				.storeOp = StorageOp::Cached
			}
		},
		.writeDepth = false
	};

	SwapchainPass::SwapchainPass():RenderPass(Name("Swapchain"), SwapchainPassDesc)
	{

	}
	void SwapchainPass::init()
	{
		RenderSystem::instance()->getRenderPassManager()->markSwapchainRenderPass(this);
		initBlitData();
	}
	SwapchainPass::~SwapchainPass()
	{
		auto RenderSys = RenderSystem::instance();
		RenderSys->destroyRsSampler(BlitRTSampler);
		delete BlitEntity;
		MaterialTemplateManager::instance()->destroyMaterialTemplate(BlitMaterial->getName());
		RenderSys->getRenderPassManager()->unMarkSwapchainRenderPass(this);

	}

	void SwapchainPass::initBlitData()
	{
		BlitEntity = new NormalEntity();
		BlitEntity->getRenderInfo().idxCount = 6;
		BlitMaterial = BlitEntity->getMaterialTemplate();
		BlitMatVarient = BlitMaterial->createVariant(this, {});
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
		RenderSys->updateUniform(BlitTarget, rt, BlitPass);
	}

	void SwapchainPass::drawImpl(rs_commandbuffer* cmdbuffer)
	{
		auto RenderSys = RenderSystem::instance();

		RenderSys->updateUniform(BlitSampler, this->BlitRTSampler, BlitPass);
		RenderSys->drawIndexed(cmdbuffer, BlitEntity, getPassName());
	}
}