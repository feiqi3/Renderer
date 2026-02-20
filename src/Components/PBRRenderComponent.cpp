#include "Components/PBRRenderComponent.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/StandardPRBRenderEntity.h"
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

	void PBRRenderComponent::onUpdate(float dt)
	{
		auto sys = RenderSystem::instance();
		RenderQueue* queue = sys->getMainRenderQueue();
		for (int i = 0; i < mMaterials.size(); ++i) {
			
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

		return entity;
	}


}

