#include "Renderer/RenderPass/PostEffectComposePass.h"
#include "Renderer/RenderEntity.h"
#include "Renderer/EnginePass.h"
#include "Renderer/MaterialInstance.h"
#include "Renderer/MaterialTemplateManager.h"
#include "Renderer/MaterialManager.h"
namespace Render {

	static inline PassDesc getPassDesc() {
		PassDesc desc{};
		PassAttachment attCol{};
		attCol.fmt = RenderTextureFormat::SwapchainFormat;
		attCol.isHDR = false;
		attCol.loadOp = StorageOp::Clear;
		attCol.storeOp = StorageOp::Cached;
		desc.attachments.push_back(attCol);
		desc.lastDepth = false;
		desc.writeDepth = false;
		return desc;
	}
	
	MaterialPtr createPostEffectComposeMaterial() {
		auto matName = Name("PostEffectCompose");
		auto mat = MaterialManager::instance()->getMaterial(matName);
		if (mat)return mat;

		RenderState postEffectRenderState{};
		postEffectRenderState.cullMode = CullMode::None;
		postEffectRenderState.depthTestEnable = false;
		postEffectRenderState.depthWriteEnable = false;


		auto matTemp = MaterialTemplateManager::instance()->createMaterialTemplate(
			Name("PostEffectCompose"), {
				{ShaderStage::Vertex,"../shader/PostEffect/FullScreen.vs"},
				{ShaderStage::Fragment, "../shader/PostEffect/PostEffectCompose.ps"}
			}, postEffectRenderState, {}
		);

		matTemp->createMaterialPass(PassName::PostEffectComposePass, { 
				{
					ShaderStage::Fragment, 
					{
						{"BLOOM",""}
					}
				}
			}
		);
		mat = MaterialManager::instance()->createMaterial<Material>(matName, matTemp);
		mat->addMaterialPassToRender(PassName::PostEffectComposePass);
		return mat;
	}

	class PostEffectComposeEntity : public RenderEntity {
	public:
		PostEffectComposeEntity() {
			this->getMaterial();
			mPass = this->createPass(PassName::PostEffectComposePass);
			auto& renderInfo = getRenderInfo();
			renderInfo.idxCount = 3;
			SamplerDesc sampDesc{};
			sampDesc.minFilter = Filter::Linear;
			sampDesc.magFilter = Filter::Linear;

			getMaterial()->bindParameter("u_colorTexSamp", SamplerResourceManager::instance()->getOrCreateSampler(sampDesc));
			sampDesc.minFilter = Filter::Nearest;
			sampDesc.magFilter = Filter::Nearest;
			getMaterial()->bindParameter("u_bloomTexSamp", SamplerResourceManager::instance()->getOrCreateSampler(sampDesc));
		}
		Material* getMaterial()override {
			if (!mMaterial) {
				mMaterial = createPostEffectComposeMaterial();
			}
			return mMaterial.get();
		}

		Pass* getPostEffectPass()const {
			return mPass;
		}
		virtual AxisAlignedBoundingBox getWorldBounding() {
			return AxisAlignedBoundingBox();
		}
	private:
		MaterialPtr mMaterial = nullptr;
		Pass* mPass = nullptr;
	};

	PostEffectComposePass::PostEffectComposePass() : RenderPass(PassName::PostEffectComposePass,getPassDesc())
	{

	}

	PostEffectComposePass::~PostEffectComposePass()
	{
		RenderSystem::instance()->destroyBuffer(mPostEffectCfgBuffer);
		delete entity;
	}

	void PostEffectComposePass::setBloomTex(const TexturePtr& tex)
	{
		ImageViewKey key;
		key.setBaseMip(0);
		entity->getMaterial()->bindParameter("u_bloomTex", tex, key);
	}

	void PostEffectComposePass::setMainRTColorTex(const TexturePtr& tex)
	{
		entity->getMaterial()->bindParameter("u_baseColorTex", tex);
	}

	void PostEffectComposePass::setBloomStrength(float strength)
	{
		mPostEffectCfg.BloomStrength = strength;
		RenderSystem::instance()->updateBufferData(mPostEffectCfgBuffer, &mPostEffectCfg, sizeof(GPUShared::PostEffectConfig),0);
	}

	void PostEffectComposePass::drawImpl(rs_commandbuffer* cmdbuffer)
	{
	}

	void PostEffectComposePass::draw(rs_commandbuffer* cmdbuffer, Camera* cam /*= nullptr*/)
	{
		auto RenderSys = RenderSystem::instance();
		RenderSys->setCurrentCamera(nullptr);
		auto nextSwapchainRt = RenderSys->getNextSwapchainRendertarget();

		RenderSys->updateParameters(cmdbuffer, this->entity, entity->getPostEffectPass());

		RenderSys->excutePendingBufferCopies(cmdbuffer);

		RenderSys->cmdBeginRenderPass(cmdbuffer, mRenderPass, mClrColor, mDsClear);
		Rect2D nextRenderArea{};
		RenderSys->cmdSetRendertarget(cmdbuffer, nextSwapchainRt);
		updateViewportAndScissor(cmdbuffer, mRendertarget);
		drawImpl(cmdbuffer);
		RenderSys->drawIndexed(cmdbuffer, entity, entity->getPostEffectPass());
	}

	void PostEffectComposePass::init()
	{
		entity = new PostEffectComposeEntity();

		BufferDesc bufferDesc{};
		bufferDesc.byteSize = sizeof(GPUShared::PostEffectConfig);
		bufferDesc.bufUsage = BufferType_Uniform;
		mPostEffectCfgBuffer = RenderSystem::instance()->createBuffer(nullptr, 0, bufferDesc);
		entity->getMaterial()->bindParameter("CBUFFER_PostEffectCfg", mPostEffectCfgBuffer);
		setBloomStrength(0.08);
	}

}