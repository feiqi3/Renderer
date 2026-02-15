#ifndef OBJECT_ENTITY_H
#define OBJECT_ENTITY_H
#include "Renderer/RenderEntity.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/RenderPass.h"
#include "Renderer/MaterialTemplateManager.h"
#include "Renderer/Texture.h"
#include "Renderer/MeshResourceManager.h"
#include"Renderer/MaterialManager.h"
#include "common/ResourceSystem.h"
namespace Render {

	class CubeEntity : public RenderEntity {
	public:
		Material* getMaterial() override {
			return mMaterial.get();
		}

		~CubeEntity() {
			auto& renderInfo = this->getRenderInfo();
		}

		CubeEntity() {
		
			auto cubeMesh = ResourceSystem::instance()->getOrCreateResource<Mesh>(ResourceName::Mesh, Name("Builtin::Cube"));
			mMaterial = ResourceSystem::instance()->getResource<Material>(ResourceName::Material, Name("NormalMaterial"));
			if (!mMaterial) {
				mMaterial = createMaterial();
			}
			auto& renderInfo = this->getRenderInfo();
			renderInfo.bindingBuffers.push_back({});
			auto& binding0 = renderInfo.bindingBuffers[0];
			binding0.buffer = cubeMesh->getVertexBuffer();
			binding0.offset = 0;

			renderInfo.indexBuffer = cubeMesh->getIndexBuffer();
			renderInfo.idxCount = cubeMesh->getIndexCount();
			renderInfo.indexType = cubeMesh->getIndexType();
			renderInfo.idxOffset = 0;
		
			this->createPass(Name("MainPass"));
		}
		virtual void updateUniforms(Pass* pass) {};


	private:
		MaterialPtr createMaterial() {
			ShaderStageInfo NomralTemplateInfo{ {ShaderStage::Vertex, "../shader/Normal.vs" }, {ShaderStage::Fragment, "../shader/Normal.ps" } };
			RenderState normalState{};
			normalState.depthTestEnable = true;
			VertexInputDescription VtxIA{
			};

			uint32_t offset = 0;
			//Vertex
			InputAttribute AttVtx{
			};
			AttVtx.binding = 0;
			AttVtx.format = VertexFormat::Float3;
			AttVtx.location = 0;
			AttVtx.offset = offset;
			VtxIA.attributes.push_back(AttVtx);
			offset += 12;

			//Normal
			AttVtx.binding = 0;
			AttVtx.format = VertexFormat::Float3;
			AttVtx.location = 1;
			AttVtx.offset = offset;
			VtxIA.attributes.push_back(AttVtx);
			offset += 12;
			//Texcoord
			AttVtx.binding = 0;
			AttVtx.format = VertexFormat::Float2;
			AttVtx.location = 2;
			AttVtx.offset = offset;
			VtxIA.attributes.push_back(AttVtx);
			offset += 8;
			InputBufferBinding binding{};
			binding.perInstance = false;
			binding.stride = offset;
			VtxIA.bindings.push_back(binding);

			auto temp =  MaterialTemplateManager::instance()->createMaterialTemplate(Name("NormalTemplate"), NomralTemplateInfo, normalState, VtxIA);
			temp->createMaterialPass(RenderSystem::instance()->getRenderPass(Name("MainCameraPass")), {});
			auto mat = MaterialManager::instance()->createMaterial<Material>(Name("NormalMaterial"),temp);
			return mat;
		}
	private:
		MaterialPtr	mMaterial = nullptr;
		MaterialPass* mMainPass = 0;

	};
};

#endif