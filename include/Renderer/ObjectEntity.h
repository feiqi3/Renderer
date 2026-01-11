#ifndef OBJECT_ENTITY_H
#define OBJECT_ENTITY_H

#include "Renderer/RenderEntity.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/RenderPass.h"
#include "Renderer/MaterialTemplateManager.h"
#include "Renderer/Texture.h"
#include "Renderer/MeshResourceManager.h"
#include "common/ResourceSystem.h"
namespace Render {

	class ObjectMaterialTemplateFactory {
	public:
		static ObjectMaterialTemplateFactory* instance() {
			static ObjectMaterialTemplateFactory Factory;
			return &Factory;
		}
	public:
		MaterialTemplate* getNormalMaterial() {
			return mNormalTemplate;
		}

		Material* getMainPass() {
			return mMainPass;
		}

	private:
		ObjectMaterialTemplateFactory() {
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

			mNormalTemplate = MaterialTemplateManager::instance()->createMaterialTemplate(Name("NormalTemplate"),NomralTemplateInfo, normalState, VtxIA);
			mMainPass = mNormalTemplate->createVariant(RenderSystem::instance()->getRenderPass(Name("MainCameraPass")), {});
		}
		MaterialTemplate* mNormalTemplate = 0;
		Material* mMainPass = 0;
	};


	class CubeEntity : public RenderEntity {
	public:
		MaterialTemplate* getMaterialTemplate() override {
			return ObjectMaterialTemplateFactory::instance()->getNormalMaterial();
		}

		~CubeEntity() {
			auto& renderInfo = this->getRenderInfo();
		}

		CubeEntity() {
		
			auto cubeMesh = ResourceSystem::instance()->getOrCreateResource<Mesh>(ResourceName::Mesh, Name("Builtin::Cube"));

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
		virtual void updateUniforms(rs_commandbuffer* cmd, Material* pass) {};


	private:
	};
};

#endif