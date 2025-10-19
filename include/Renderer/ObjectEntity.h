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

		CubeEntity() {
		
			static std::vector<float> cubeVtx = {
				// 位置              // 法线              // 纹理坐标
				// 前面
				-1.0f, -1.0f,  1.0f,   0.0f,  0.0f,  1.0f,   0.0f, 0.0f,
				 1.0f, -1.0f,  1.0f,   0.0f,  0.0f,  1.0f,   1.0f, 0.0f,
				 1.0f,  1.0f,  1.0f,   0.0f,  0.0f,  1.0f,   1.0f, 1.0f,
				-1.0f, -1.0f,  1.0f,   0.0f,  0.0f,  1.0f,   0.0f, 0.0f,
				 1.0f,  1.0f,  1.0f,   0.0f,  0.0f,  1.0f,   1.0f, 1.0f,
				-1.0f,  1.0f,  1.0f,   0.0f,  0.0f,  1.0f,   0.0f, 1.0f,

				// 背面
				-1.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f,   1.0f, 0.0f,
				-1.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f,   1.0f, 1.0f,
				 1.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f,   0.0f, 1.0f,
				-1.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f,   1.0f, 0.0f,
				 1.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f,   0.0f, 1.0f,
				 1.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f,   0.0f, 0.0f,

				 // 左面
				 -1.0f, -1.0f, -1.0f,  -1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
				 -1.0f, -1.0f,  1.0f,  -1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
				 -1.0f,  1.0f,  1.0f,  -1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
				 -1.0f, -1.0f, -1.0f,  -1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
				 -1.0f,  1.0f,  1.0f,  -1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
				 -1.0f,  1.0f, -1.0f,  -1.0f,  0.0f,  0.0f,   0.0f, 1.0f,

				 // 右面
				  1.0f, -1.0f, -1.0f,   1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
				  1.0f,  1.0f,  1.0f,   1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
				  1.0f, -1.0f,  1.0f,   1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
				  1.0f, -1.0f, -1.0f,   1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
				  1.0f,  1.0f, -1.0f,   1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
				  1.0f,  1.0f,  1.0f,   1.0f,  0.0f,  0.0f,   0.0f, 1.0f,

				  // 顶面
				  -1.0f,  1.0f, -1.0f,   0.0f,  1.0f,  0.0f,   0.0f, 1.0f,
				  -1.0f,  1.0f,  1.0f,   0.0f,  1.0f,  0.0f,   0.0f, 0.0f,
				   1.0f,  1.0f,  1.0f,   0.0f,  1.0f,  0.0f,   1.0f, 0.0f,
				  -1.0f,  1.0f, -1.0f,   0.0f,  1.0f,  0.0f,   0.0f, 1.0f,
				   1.0f,  1.0f,  1.0f,   0.0f,  1.0f,  0.0f,   1.0f, 0.0f,
				   1.0f,  1.0f, -1.0f,   0.0f,  1.0f,  0.0f,   1.0f, 1.0f,

				   // 底面
				   -1.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f,   1.0f, 1.0f,
					1.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f,   0.0f, 0.0f,
				   -1.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f,   1.0f, 0.0f,
				   -1.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f,   1.0f, 1.0f,
					1.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f,   0.0f, 1.0f,
					1.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f,   0.0f, 0.0f
			};
			static std::vector<uint32_t> cubeIndices = {
				// 前面
				4, 5, 6,
				4, 6, 7,

				// 背面
				1, 0, 3,
				1, 3, 2,

				// 左面
				0, 4, 7,
				0, 7, 3,

				// 右面
				5, 1, 2,
				5, 2, 6,

				// 顶面
				3, 7, 6,
				3, 6, 2,

				// 底面
				0, 1, 5,
				0, 5, 4
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
			renderInfo.indexBuffer = RenderSystem::instance()->createBuffer(cubeIndices.data(), bufferSize, desc);
		
			this->setMaterialTemplate(ObjectMaterialTemplateFactory::instance()->getNormalMaterial());


		}
			
	private:

	};
};

#endif