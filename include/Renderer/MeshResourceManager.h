#ifndef MESH_RESOURCE_MANAGER_H_
#define MESH_RESOURCE_MANAGER_H_

#include "common/ResourceManager.h"
#include "Renderer/Mesh.h" 

namespace Render {

	class MeshResourceManager : public ResourceManager<Mesh> {
	public:
		MeshResourceManager();
		virtual const Name& typeName() const override;

	private:
		void createNecessaryPersistenceResources() override;

	protected:
		virtual Mesh* loadImpl(const Name& id) override;
		virtual void  unloadImpl(Mesh* mesh) override;
	};
}

#endif // MESH_RESOURCE_MANAGER_H_