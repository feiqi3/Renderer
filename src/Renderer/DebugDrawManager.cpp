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

		struct alignas(4) LineInfo {
			vec3	begin;	//12
			u32		color;	//4
			vec3	end;	//12
			float	width;	//4
		};
	}
	class LineDrawEntity : public RenderEntity {
	public:
		MaterialPtr material;
		
		LineDrawEntity() {
			this->getRenderInfo().bindingBuffers.resize(0);

			auto& renderInfo = this->getRenderInfo();
			renderInfo.bindingBuffers.resize(1, {});
			renderInfo.idxCount = 6;
		}

		void setInstanceCount(int i) {
			this->getRenderInfo().instanceCount = i;
		}

		virtual AxisAlignedBoundingBox getWorldBounding() override {
			return AxisAlignedBoundingBox();
		}

		void setPerInstanceBuffer(rs_buffer* buffer) {
			this->getRenderInfo().bindingBuffers[0].buffer = buffer;
		}

		virtual Material* getMaterial() override {
			return material.get();
		}
	};

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

		MaterialTemplatePtr debugDrawLineTemp;
		MaterialPtr			debugDrawLineMat;

		bool init = false;

		std::unique_ptr<DebugDrawEntity> mCubeEntity;
		rs_buffer* mCubePerInstanceBuffer = nullptr;
		int cubeInsBufferNumber = 0;
		std::vector<PerObjectInfo> mCubeInfo;

		std::unique_ptr<DebugDrawEntity> mQuadEntity;
		rs_buffer* mQuadPerInstanceBuffer = nullptr;
		int quadInsBufferNumber = 0;
		std::vector<PerObjectInfo> mQuadInfo;

		std::unique_ptr<LineDrawEntity> mLineEntity;
		rs_buffer* mLinePerInstanceBuffer = nullptr;
		int lineInsBufferNumber = 0;
		std::vector<LineInfo> mLineInfo;
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

	void DebugDrawManager::drawCube(const vec3& pos, const quat& rotation, const vec4& color, float size)
	{
		PerObjectInfo info{};
		mat4 id = mat4(1.0f);
		info.color = color;
		info.world = getTRS(pos, rotation, vec3(size));
		info.color = color;
		info.useBillboard = 0.;
		mDp->mCubeInfo.push_back(info);
	}

	void DebugDrawManager::drawPoint(const vec3& pos, const vec4& color, float size)
	{
		PerObjectInfo info;
		mat4 id = mat4(1.0f);
		info.world = glm::translate(id, pos) * glm::scale(id, vec3(0.03f) * size);
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

	void DebugDrawManager::drawLine(const vec3& beg, const vec3& end, const vec4& color, float width)
	{
		union {
			struct {
				u8 r;
				u8 g;
				u8 b;
				u8 a;
			};
			uint32_t c;
		};
		r = u8( color.r * 255.f );
		g = u8( color.g * 255.f );
		b = u8( color.b * 255.f );
		a = u8( color.a * 255.f );
		
		LineInfo line{};
		line.begin = beg;
		line.end = end;
		line.width = width;
		line.color = c;
		mDp->mLineInfo.push_back(line);
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

		if (!mDp->mLineInfo.empty()) {
			bool bufferUpdated = false;
			auto sizeByte = sizeof(LineInfo) * mDp->mLineInfo.size();
			if (mDp->mLinePerInstanceBuffer == nullptr || mDp->lineInsBufferNumber < mDp->mLineInfo.size()) {
				if(mDp->mLinePerInstanceBuffer){
					RenderSystem::instance()->destroyBuffer(mDp->mLinePerInstanceBuffer);
				}
				BufferDesc desc{};
				desc.bufUsage = BufferType_Vertex;
				desc.byteSize = sizeByte;
				desc.queueType = QueueType_Graphics;
				mDp->mLinePerInstanceBuffer = RenderSystem::instance()->createBuffer(mDp->mLineInfo.data(), sizeByte, desc);
				bufferUpdated = true;
				mDp->lineInsBufferNumber = mDp->mQuadInfo.size();
			}

			if (!bufferUpdated) {
				RenderSystem::instance()->updateBufferData(mDp->mLinePerInstanceBuffer, mDp->mLineInfo.data(), sizeByte, 0);
			}

			mDp->mLineEntity->setPerInstanceBuffer(mDp->mLinePerInstanceBuffer);
			mDp->mLineEntity->setInstanceCount(mDp->mLineInfo.size());
			cam->getRenderQueue()->submit(mDp->mLineEntity.get(), RenderMask::DebugDraw);
			mDp->mLineInfo.clear();
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

		{
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

		{
			stageInfo[0].second = "../shader/DebugDrawLine.vs";
			state.blendStates.clear();
			state.depthWriteEnable = true;
			state.depthTestEnable = true;
			desc.bindings.clear();
			InputBufferBinding bindingInfo{};
			bindingInfo.perInstance = true;
			bindingInfo.stride = sizeof(LineInfo);
			desc.bindings.push_back(bindingInfo);
			InputAttribute attr{};
			uint32_t offset = 0;
			attr.binding = 0;
			
			desc.attributes.clear();

			attr.location = 0;
			attr.format = VertexFormat::Float3;
			attr.offset = offsetof(LineInfo, begin);
			desc.attributes.push_back(attr);

			attr.location = 1;
			attr.offset = offsetof(LineInfo, end);
			attr.format = VertexFormat::Float3;
			desc.attributes.push_back(attr);

			attr.location = 2;
			attr.format = VertexFormat::UByte4N;
			attr.offset = offsetof(LineInfo, color);
			desc.attributes.push_back(attr);

			attr.location = 3;
			attr.offset = offsetof(LineInfo, width);
			attr.format = VertexFormat::Float;
			desc.attributes.push_back(attr);

		}

		{
			auto matTemp = MaterialTemplateManager::instance()->createMaterialTemplate(
				Name("DrawLineMaterialTemplate"), stageInfo, state, desc
			);
			mDp->debugDrawLineTemp = matTemp;
			matTemp->createMaterialPass(PassName::MainCameraPass);
			mDp->debugDrawLineMat = MaterialManager::instance()->createMaterial<Material>(Name("DrawLineMaterial"), matTemp);
			mDp->debugDrawLineMat->setRenderMask(RenderMask::DebugDraw);

			mDp->mLineEntity = std::make_unique<LineDrawEntity>();
			mDp->mLineEntity->material = mDp->debugDrawLineMat;
			mDp->mLineEntity->setRenderMask(RenderMask::DebugDraw);
		}

	}

	void DebugDrawManager::init()
	{
		if (mDp->init) return;
		mDp->init = true;
		initDebugDrawInfo();
	}

}