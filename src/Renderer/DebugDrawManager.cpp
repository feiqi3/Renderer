#include "Renderer/DebugDrawManager.h"
#include "Renderer/RenderEntity.h"
#include "Renderer/MaterialInstance.h"
#include "Renderer/MaterialTemplateManager.h"
#include "Renderer/ModelVertex.h"
#include "Renderer/EnginePass.h"
#include "Renderer/MaterialManager.h"
#include "Renderer/MeshResourceManager.h"
#include "Renderer/Mesh.h"
namespace Render {
	namespace {
		struct PerObjectInfo {
			mat4 world;
			vec4 color;
		};
	}

	class DebugDrawEntity : public RenderEntity{
	public:
		MaterialPtr material;
		MeshPtr cubeMesh;
		DebugDrawEntity() {
			this->getRenderInfo().bindingBuffers.resize(2);
			cubeMesh = ResourceSystem::instance()->getResource<Mesh>(Mesh::typeName(),Name("Builtin::Cube"));
			auto& renderInfo = this->getRenderInfo();
			renderInfo.bindingBuffers.resize(2, {});
			renderInfo.bindingBuffers[0].buffer = cubeMesh->getVertexBuffer();
			renderInfo.indexBuffer = cubeMesh->getIndexBuffer();
			renderInfo.indexType = cubeMesh->getIndexType();
			renderInfo.idxCount = cubeMesh->getIndexCount();
		}

		void setInstanceCount(int i) {
			this->getRenderInfo().instanceCount = i;
		}

		virtual AxisAlignedBoundingBox getWorldBounding()override {
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
		std::unique_ptr<DebugDrawEntity> mPointEntity;
		bool init = false;
		rs_buffer* mPointPerInstanceBuffer = nullptr;
		int pointNumber = 0;
		std::vector<PerObjectInfo> mPointInfo;
	};

	DebugDrawManager::DebugDrawManager()
	{
		mDp = std::make_unique<DebugDrawManagerPrivate>();
	}

	DebugDrawManager::~DebugDrawManager()
	{
		mDp = nullptr;
	}


void DebugDrawManager::drawPoint(const vec3& pos, const vec4 color)
{
	PerObjectInfo info;
	mat4 id = mat4(1.);
	info.world = translate(id, pos);
	info.color = color;
	mDp->mPointInfo.push_back(info);
}

void DebugDrawManager::drawAABB(AxisAlignedBoundingBox& aabb)
{

}

void DebugDrawManager::onRender(Camera* cam)
{
	//1. create buffer 
}

void DebugDrawManager::initDebugDrawInfo()
{
	//1. create material template
	ShaderStageInfo stageInfo = {
		{ShaderStage::Vertex,		"DebugDraw.vs"},
		{ShaderStage::Fragment,		"DebugDraw.ps"},
	};
	RenderState state{};
	VertexInputDescription desc{};
	{
		desc.bindings.push_back(InputBufferBinding{ .stride = sizeof(StandardModelVertex),.perInstance = false });
		desc.bindings.push_back(InputBufferBinding{ .stride = sizeof(PerObjectInfo),.perInstance = true });
		auto& IADescVec = desc.attributes;
		InputAttribute attr{};
		attr.binding	= 0;
		attr.location	= 0;
		attr.format		= VertexFormat::Float3;
		attr.offset		= 0;
		IADescVec.push_back(attr);
		attr.binding	= 1;
		attr.location	= 1;
		attr.format		= VertexFormat::Mat4;
		attr.offset		= 0;
		IADescVec.push_back(attr);

		attr.binding	= 1;
		attr.location	= 2;
		attr.format = VertexFormat::Float4;
		attr.offset = offsetof(PerObjectInfo, color);
		IADescVec.push_back(attr);
	}
	auto matTemp = MaterialTemplateManager::instance()->createMaterialTemplate(
		Name("DebugDrawMaterialTemplate"), stageInfo, state, desc
	);
	mDp->debugDrawTemp = matTemp;
	matTemp->createMaterialPass(PassName::MainCameraPass);
	mDp->debugDrawMat = MaterialManager::instance()->createMaterial<Material>(Name("DebugDrawMaterial"), matTemp);
	
	mDp->mPointEntity = std::make_unique<DebugDrawEntity>();
	mDp->mPointEntity->material = mDp->debugDrawMat;
}

void DebugDrawManager::init()
{
	if (mDp->init)return;
	mDp->init = true;
	initDebugDrawInfo();
}

}
