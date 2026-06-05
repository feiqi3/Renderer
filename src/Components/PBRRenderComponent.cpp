#include "Components/PBRRenderComponent.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/EnginePass.h"
#include "Renderer/StandardPRBRenderEntity.h"
#include "Renderer/RenderQueue.h"
#include "function/Object.h"
namespace Render {
	void PBRRenderComponent::onAttach()
	{
		
	}

	void PBRRenderComponent::setMesh(const MeshPtr& mesh)
	{
		mMesh = mesh;
		this->mMaterials.resize(mesh->getSubMeshCount());
	}

	void PBRRenderComponent::setMaterial(int submeshId, MaterialPtr mat)
	{
		if (submeshId < 0 || submeshId >= mMaterials.size()) {
			return;
		}
		mMaterials[submeshId] = mat;
	}

	MeshPtr PBRRenderComponent::getMesh(const MeshPtr& mesh)
	{
		return mMesh;
	}

	MaterialPtr PBRRenderComponent::getMaterial(int submeshID, MaterialPtr& mat)
	{
		if (submeshID < 0 || submeshID >= mMaterials.size()) {
			return nullptr;
		}
		return mMaterials[submeshID];
	}

	void PBRRenderComponent::collectRenderEntities(std::vector<RenderEntity*>& outEntities) const
	{
		for (RenderEntity* entity : mRenderEntities) {
			outEntities.push_back(entity);
		}
	}

	void PBRRenderComponent::onUpdate(float dt)
	{

	}

	void PBRRenderComponent::onDestroy()
	{
		for (auto&& i : mRenderEntities) {
			delete i;
			i = 0;
		}
	}

	PBRRenderComponent::~PBRRenderComponent()
	{
		for (auto entity : mRenderEntities) {
			delete entity;
		}
		mRenderEntities.clear();
	}

	void PBRRenderComponent::updateRenderEntities()
	{

	}

	RenderEntity* PBRRenderComponent::createRenderEntity(int submeshID, MaterialPtr mat)
	{
		StandardPBRRenderEntity* entity = new StandardPBRRenderEntity;
		auto& info = entity->getRenderInfo();
		entity->setPBRMaterial(mat);
		const auto& subMesh = this->mMesh->getSubMesh(submeshID);
		info.idxCount = subMesh.indexCount;
		info.idxOffset = subMesh.indexOffset;
		info.indexType = mMesh->getIndexType();
		info.instanceCount = 1;
		info.vtxoffset = subMesh.vertexOffset;
		info.indexBuffer = mMesh->getIndexBuffer();
		info.bindingBuffers.resize(1);
		info.bindingBuffers[0].buffer = mMesh->getVertexBuffer();
		info.bindingBuffers[0].offset = 0;
		
		auto* mainPass = mat->getMaterialPassToRender(PassName::MainCameraPass);
		assert(mainPass != nullptr);
		entity->createPass(PassName::MainCameraPass);
		return entity;
	}


}

