#include "Components/SimpleRenderComponent.h"
#include "Renderer/RenderEntity.h"
#include "Renderer/MaterialInstance.h"
#include "Renderer/MaterialManager.h"
#include "Renderer/MaterialTemplateManager.h"
#include "Renderer/EnginePass.h"
#include "common/ResourceSystem.h"
#include "Renderer/RenderPassManager.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/Texture.h"
#include "Renderer/SamplerResourceManager.h"
#include "Renderer/MeshResourceManager.h"
#include "function/Object.h"
#include "Renderer/RenderQueue.h"
namespace Render {
	class SimpleCubeRenderEntity : public RenderEntity{
	public:
		SimpleCubeRenderEntity() {
			Name materialName = Name("SimpleCube");
			auto materialManager = MaterialManager::instance();
			mMaterial = materialManager->getMaterial(materialName);
			if (nullptr == mMaterial) {
				auto cubeMatTemplate = MaterialTemplateManager::instance()->getMaterialTemplate(Name("Builtin::CubeMateralTemplate"));
				assert(cubeMatTemplate != nullptr && "MISSING BAISC MATERIAL TEMPLATE");
				if (cubeMatTemplate) {
					auto pass = cubeMatTemplate->getMaterialPass(PassName::MainCameraPass);
					if (!pass) {
						cubeMatTemplate->createMaterialPass(RenderSystem::instance()->getRenderPass(PassName::MainCameraPass), {});
					}
				}
				mMaterial = materialManager->createMaterial<Material>(materialName, cubeMatTemplate);
				mMaterial->addMaterialPassToRender(PassName::MainCameraPass);
				auto texturePtr = ResourceSystem::instance()->getResource<Texture>(Texture::typeName(),Name("Builtin::ErrorRGB"));
				mMaterial->bindParameter(
					"BoxTex", texturePtr
				);

				SamplerDesc samplerDesc{};
				samplerDesc.addressU = AddressMode::Repeat;
				samplerDesc.addressV = AddressMode::Repeat;
				auto samplerPtr = SamplerResourceManager::instance()->getOrCreateSampler(
					samplerDesc
				);

				mMaterial->bindParameter(
					"BoxTex", texturePtr
				);

				mMaterial->bindParameter(
					"BoxSampler", samplerPtr
				);
			}

		}
		virtual void updateUniforms(Pass* pass) override {
			//Do Nothing.
		}


		Material* getMaterial()override {
			return mMaterial.get();
		};
	private:
		MaterialPtr mMaterial;
	};

	void Render::SimpleRenderComponent::onUpdate(float dt)
	{
		if (this->mRenderEntity == nullptr) {
			this->mRenderEntity = new SimpleCubeRenderEntity();
		}
		
		mRenderEntity->setModelMatrix(
			this->owner()->worldMatrix()
		);
		RenderSystem::instance()->getMainRenderQueue()->submit(
			mRenderEntity
		);
	}

	void SimpleRenderComponent::onDestroy()
	{
		delete mRenderEntity;
		mRenderEntity = nullptr;
	}

}
