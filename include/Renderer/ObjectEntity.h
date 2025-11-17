#ifndef OBJECT_ENTITY_H
#define OBJECT_ENTITY_H

#include "Renderer/RenderEntity.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/RenderPass.h"
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
			AttVtx.format = VertexFormat::RGB32_SFLOAT;
			AttVtx.location = 0;
			AttVtx.offset = offset;
			VtxIA.attributes.push_back(AttVtx);
			offset += 12;

			//Normal
			AttVtx.binding = 0;
			AttVtx.format = VertexFormat::RGB32_SFLOAT;
			AttVtx.location = 1;
			AttVtx.offset = offset;
			VtxIA.attributes.push_back(AttVtx);
			offset += 12;
			//Texcoord
			AttVtx.binding = 0;
			AttVtx.format = VertexFormat::RG32_SFLOAT;
			AttVtx.location = 2;
			AttVtx.offset = offset;
			VtxIA.attributes.push_back(AttVtx);
			offset += 8;
			InputBufferBinding binding{};
			binding.perInstance = false;
			binding.stride = offset;
			VtxIA.bindings.push_back(binding);
			mNormalTemplate = new MaterialTemplate(NomralTemplateInfo, normalState, VtxIA);
			mMainPass = mNormalTemplate->createVarient(RenderSystem::instance()->getRenderPass("MainPass"), {});
		}
		MaterialTemplate* mNormalTemplate = 0;
		Material* mMainPass = 0;
	};


	class CubeEntity : public RenderEntity {
	public:
		MaterialTemplate* getMaterial() override {
			return ObjectMaterialTemplateFactory::instance()->getNormalMaterial();
		}

		~CubeEntity() {
			auto& renderInfo = this->getRenderInfo();
			RenderSystem::instance()->destroyBuffer(renderInfo.indexBuffer);
			RenderSystem::instance()->destroyBuffer(renderInfo.bindingBuffers[0].buffer);
		}

		CubeEntity() {
		
			static std::vector<float> cubeVtx = {
				-0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
				 0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
				 0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
				-0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,

				// ---- Back face ----
				-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,-1.0f,  1.0f, 0.0f,
				 0.5f, -0.5f, -0.5f,  0.0f, 0.0f,-1.0f,  0.0f, 0.0f,
				 0.5f,  0.5f, -0.5f,  0.0f, 0.0f,-1.0f,  0.0f, 1.0f,
				-0.5f,  0.5f, -0.5f,  0.0f, 0.0f,-1.0f,  1.0f, 1.0f,

				// ---- Left face ----
				-0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
				-0.5f, -0.5f,  0.5f, -1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
				-0.5f,  0.5f,  0.5f, -1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
				-0.5f,  0.5f, -0.5f, -1.0f, 0.0f, 0.0f,  0.0f, 1.0f,

				// ---- Right face ----
				 0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
				 0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
				 0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
				 0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,

				 // ---- Bottom face ----
				 -0.5f, -0.5f, -0.5f,  0.0f,-1.0f, 0.0f,  0.0f, 1.0f,
				  0.5f, -0.5f, -0.5f,  0.0f,-1.0f, 0.0f,  1.0f, 1.0f,
				  0.5f, -0.5f,  0.5f,  0.0f,-1.0f, 0.0f,  1.0f, 0.0f,
				 -0.5f, -0.5f,  0.5f,  0.0f,-1.0f, 0.0f,  0.0f, 0.0f,

				 // ---- Top face ----
				 -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
				  0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
				  0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
				 -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
			};
			static std::vector<uint32_t> cubeIndices = {
				0, 1, 2,  2, 3, 0,        // Front
				4, 5, 6,  6, 7, 4,        // Back
				8, 9,10, 10,11, 8,        // Left
			   12,13,14, 14,15,12,        // Right
			   16,17,18, 18,19,16,        // Bottom
			   20,21,22, 22,23,20         // Top
			};

			auto& renderInfo = this->getRenderInfo();
			uint32_t bufferSize = cubeVtx.size() * sizeof(float);
			BufferDesc desc{};
			desc.bufUsage = BufferType_Vertex;
			desc.byteSize = cubeVtx.size() * sizeof(float);
			desc.mappable = false;
			desc.queueType = QueueType_Graphics;
			renderInfo.bindingBuffers.push_back({});
			auto& binding0 = renderInfo.bindingBuffers[0];
			binding0.buffer = RenderSystem::instance()->createBuffer(cubeVtx.data(), bufferSize, desc);
			binding0.offset = 0;

			bufferSize = cubeIndices.size() * sizeof(float);
			desc.bufUsage = BufferType_Index;
			desc.byteSize = bufferSize;
			renderInfo.indexBuffer = RenderSystem::instance()->createBuffer(cubeIndices.data(), bufferSize, desc);
			renderInfo.idxCount = cubeIndices.size();
			renderInfo.indexType = IndexType::Uint32;
		
			this->setMaterialTemplate(ObjectMaterialTemplateFactory::instance()->getNormalMaterial());
			this->createPass("MainPass");

		}
			
	private:

	};
};

#endif