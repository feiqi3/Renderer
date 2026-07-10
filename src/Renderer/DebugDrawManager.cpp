#include "Renderer/DebugDrawManager.h"
#include "Renderer/RenderEntity.h"
#include "Renderer/MaterialInstance.h"
#include "Renderer/MaterialTemplateManager.h"
#include "Renderer/ModelVertex.h"
#include "Renderer/EnginePass.h"
#include "Renderer/MaterialManager.h"
#include "Renderer/MeshResourceManager.h"
#include "Renderer/Mesh.h"
#include "Renderer/Camera.h"

namespace Render {
	namespace {
		struct PerObjectInfo {
			mat4 world;
			vec4 color;
			float useBillboard = -1.;
		};
	}

	class DebugDrawEntity : public RenderEntity {
	public:
		MaterialPtr material;
		MeshPtr mesh;

		DebugDrawEntity(const Name& meshName) {
			this->getRenderInfo().bindingBuffers.resize(2);
			mesh = ResourceSystem::instance()->getResource<Mesh>(Mesh::typeName(), meshName);

			auto& renderInfo = this->getRenderInfo();
			renderInfo.bindingBuffers.resize(2, {});
			renderInfo.bindingBuffers[0].buffer = mesh->getVertexBuffer();
			renderInfo.indexBuffer = mesh->getIndexBuffer();
			renderInfo.indexType = mesh->getIndexType();
			renderInfo.idxCount = mesh->getIndexCount();
		}

		void setInstanceCount(int i) {
			this->getRenderInfo().instanceCount = i;
		}

		virtual AxisAlignedBoundingBox getWorldBounding() override {
			return AxisAlignedBoundingBox();
		}

		void setPerInstanceBuffer(rs_buffer* buffer) {
			this->getRenderInfo().bindingBuffers[1].buffer = buffer;
		}

		virtual Material* getMaterial() override {
			return material.get();
		}
	};

	class DebugDrawManagerPrivate {
	public:
		MaterialTemplatePtr debugDrawTemp;
		MaterialPtr			debugDrawMat;
		bool init = false;

		std::unique_ptr<DebugDrawEntity> mCubeEntity;
		rs_buffer* mCubePerInstanceBuffer = nullptr;
		int cubeInsBufferNumber = 0;
		std::vector<PerObjectInfo> mCubeInfo;

		std::unique_ptr<DebugDrawEntity> mQuadEntity;
		rs_buffer* mQuadPerInstanceBuffer = nullptr;
		int quadInsBufferNumber = 0;
		std::vector<PerObjectInfo> mQuadInfo;
	};

	DebugDrawManager::DebugDrawManager()
	{
		mDp = std::make_unique<DebugDrawManagerPrivate>();
	}

	DebugDrawManager::~DebugDrawManager()
	{
		if (mDp->mCubePerInstanceBuffer) {
			RenderSystem::instance()->destroyBuffer(mDp->mCubePerInstanceBuffer);
		}
		if (mDp->mQuadPerInstanceBuffer) {
			RenderSystem::instance()->destroyBuffer(mDp->mQuadPerInstanceBuffer);
		}
		mDp = nullptr;
	}

	void DebugDrawManager::drawPoint(const vec3& pos, const vec4& color)
	{
		PerObjectInfo info;
		mat4 id = mat4(1.0f);
		info.world = glm::translate(id, pos) * glm::scale(id, vec3(0.03f));
		info.color = color;
		info.useBillboard = 1.;
		mDp->mQuadInfo.push_back(info);
	}

	void DebugDrawManager::drawQuad(const vec3& center, const vec2& size, const vec4& color)
	{
		PerObjectInfo info;
		mat4 id = mat4(1.0f);
		info.world = glm::translate(id, center) * glm::scale(id, vec3(size.x, size.y, 1.0f));
		info.color = color;
		mDp->mQuadInfo.push_back(info);
	}

	void DebugDrawManager::drawPlane(const Plane& plane, const vec2& size, const vec4& color)
	{
		vec3 normal = glm::normalize(plane.getNormal());
		vec3 defaultCenter = -normal * plane.getDistance();

		drawPlane(plane, defaultCenter, size, color);
	}

	void DebugDrawManager::drawPlane(const Plane& plane, const vec3& center, const vec2& size, const vec4& color)
	{
		vec3 normal = glm::normalize(plane.getNormal());

		vec3 up = (std::abs(normal.y) < 0.999f) ? vec3(0.0f, 1.0f, 0.0f) : vec3(1.0f, 0.0f, 0.0f);

		vec3 tangent = glm::normalize(glm::cross(up, normal));
		vec3 bitangent = glm::cross(normal, tangent);

		mat4 rotation = mat4(1.0f);
		rotation[0] = vec4(tangent, 0.0f);    
		rotation[1] = vec4(bitangent, 0.0f);  
		rotation[2] = vec4(normal, 0.0f);     

		mat4 id = mat4(1.0f);
		PerObjectInfo info;
		info.world = glm::translate(id, center) * rotation * glm::scale(id, vec3(size.x, size.y, 1.0f));
		info.color = color;
		info.useBillboard = -1; 

		mDp->mQuadInfo.push_back(info);
	}

	void DebugDrawManager::drawAABB(const AxisAlignedBoundingBox& aabb, const vec4& color)
	{
		mat4 id = mat4(1.0f);
		vec3 scaleSize = aabb.getSize();
		vec3 center = aabb.getCenter();

		PerObjectInfo info;
		info.world = glm::translate(id, center) * glm::scale(id, scaleSize);
		info.color = color;
		mDp->mCubeInfo.push_back(info);
	}

	void DebugDrawManager::onRender(Camera* cam)
	{
		if (!mDp->mCubeInfo.empty()) {
			bool bufferUpdated = false;
			auto sizeByte = sizeof(PerObjectInfo) * mDp->mCubeInfo.size();
			if (mDp->mCubePerInstanceBuffer == nullptr || mDp->cubeInsBufferNumber < mDp->mCubeInfo.size()) {
				RenderSystem::instance()->destroyBuffer(mDp->mCubePerInstanceBuffer);
				BufferDesc desc{};
				desc.bufUsage = BufferType_Vertex;
				desc.byteSize = sizeByte;
				desc.queueType = QueueType_Graphics;
				mDp->mCubePerInstanceBuffer = RenderSystem::instance()->createBuffer(mDp->mCubeInfo.data(), sizeByte, desc);
				bufferUpdated = true;
				mDp->cubeInsBufferNumber = mDp->mCubeInfo.size();
			}

			if (!bufferUpdated) {
				RenderSystem::instance()->updateBufferData(mDp->mCubePerInstanceBuffer, mDp->mCubeInfo.data(), sizeByte, 0);
			}

			mDp->mCubeEntity->setPerInstanceBuffer(mDp->mCubePerInstanceBuffer);
			mDp->mCubeEntity->setInstanceCount(mDp->mCubeInfo.size());
			cam->getRenderQueue()->submit(mDp->mCubeEntity.get(), RenderMask::DebugDraw);
			mDp->mCubeInfo.clear();
		}

		if (!mDp->mQuadInfo.empty()) {
			bool bufferUpdated = false;
			auto sizeByte = sizeof(PerObjectInfo) * mDp->mQuadInfo.size();
			if (mDp->mQuadPerInstanceBuffer == nullptr || mDp->quadInsBufferNumber < mDp->mQuadInfo.size()) {
				RenderSystem::instance()->destroyBuffer(mDp->mQuadPerInstanceBuffer);
				BufferDesc desc{};
				desc.bufUsage = BufferType_Vertex;
				desc.byteSize = sizeByte;
				desc.queueType = QueueType_Graphics;
				mDp->mQuadPerInstanceBuffer = RenderSystem::instance()->createBuffer(mDp->mQuadInfo.data(), sizeByte, desc);
				bufferUpdated = true;
				mDp->quadInsBufferNumber = mDp->mQuadInfo.size();
			}

			if (!bufferUpdated) {
				RenderSystem::instance()->updateBufferData(mDp->mQuadPerInstanceBuffer, mDp->mQuadInfo.data(), sizeByte, 0);
			}

			mDp->mQuadEntity->setPerInstanceBuffer(mDp->mQuadPerInstanceBuffer);
			mDp->mQuadEntity->setInstanceCount(mDp->mQuadInfo.size());
			cam->getRenderQueue()->submit(mDp->mQuadEntity.get(), RenderMask::DebugDraw);
			mDp->mQuadInfo.clear();
		}
	}

	void DebugDrawManager::initDebugDrawInfo()
	{
		ShaderStageInfo stageInfo = {
			{ShaderStage::Vertex,		"../shader/DebugDraw.vs"},
			{ShaderStage::Fragment,		"../shader/DebugDraw.ps"},
		};
		RenderState state{};
		BlendState blendInfo{};
		blendInfo.blendEnable = true;
		blendInfo.colorBlendOp = BlendOp::Add;
		
		blendInfo.srcAlphaBlend = BlendFactor::One;
		blendInfo.dstAlphaBlend = BlendFactor::OneMinusSrcAlpha;

		blendInfo.srcColorBlend = BlendFactor::SrcAlpha;
		blendInfo.dstColorBlend = BlendFactor::OneMinusSrcAlpha;
		
		state.blendStates.push_back(blendInfo);
		VertexInputDescription desc{};
		{
			desc.bindings.push_back(InputBufferBinding{ .stride = sizeof(StandardModelVertex),.perInstance = false });
			desc.bindings.push_back(InputBufferBinding{ .stride = sizeof(PerObjectInfo),.perInstance = true });
			auto& IADescVec = desc.attributes;
			InputAttribute attr{};
			attr.binding = 0;
			attr.location = 0;
			attr.format = VertexFormat::Float3;
			attr.offset = 0;
			IADescVec.push_back(attr);

			attr.binding = 1;
			attr.location = 1;
			attr.format = VertexFormat::Float4;
			attr.offset = 0;
			IADescVec.push_back(attr);

			attr.binding = 1;
			attr.location = 2;
			attr.format = VertexFormat::Float4;
			attr.offset = sizeof(vec4) * 1;
			IADescVec.push_back(attr);

			attr.binding = 1;
			attr.location = 3;
			attr.format = VertexFormat::Float4;
			attr.offset = sizeof(vec4) * 2;
			IADescVec.push_back(attr);

			attr.binding = 1;
			attr.location = 4;
			attr.format = VertexFormat::Float4;
			attr.offset = sizeof(vec4) * 3;
			IADescVec.push_back(attr);

			attr.binding = 1;
			attr.location = 5;
			attr.format = VertexFormat::Float4;
			attr.offset = offsetof(PerObjectInfo, color);
			IADescVec.push_back(attr);

			attr.binding = 0;
			attr.location = 6;
			attr.format = VertexFormat::Float3;
			attr.offset = offsetof(StandardModelVertex, normal);
			IADescVec.push_back(attr);

			attr.binding = 1;
			attr.location = 7;
			attr.format = VertexFormat::Float;
			attr.offset = offsetof(PerObjectInfo, useBillboard);
			IADescVec.push_back(attr);
		}

		auto matTemp = MaterialTemplateManager::instance()->createMaterialTemplate(
			Name("DebugDrawMaterialTemplate"), stageInfo, state, desc
		);
		mDp->debugDrawTemp = matTemp;
		matTemp->createMaterialPass(PassName::MainCameraTransparentPass);
		mDp->debugDrawMat = MaterialManager::instance()->createMaterial<Material>(Name("DebugDrawMaterial"), matTemp);
		mDp->debugDrawMat->setRenderMask(RenderMask::DebugDraw);

		mDp->mCubeEntity = std::make_unique<DebugDrawEntity>(Name("Builtin::Cube"));
		mDp->mCubeEntity->material = mDp->debugDrawMat;
		mDp->mCubeEntity->setRenderMask(RenderMask::DebugDraw);

		mDp->mQuadEntity = std::make_unique<DebugDrawEntity>(Name("Builtin::Quad"));
		mDp->mQuadEntity->material = mDp->debugDrawMat;
		mDp->mQuadEntity->setRenderMask(RenderMask::DebugDraw);
	}

	void DebugDrawManager::init()
	{
		if (mDp->init) return;
		mDp->init = true;
		initDebugDrawInfo();
	}

}