#ifndef MODEL_RESOURCE_MANAGER_H
#define MODEL_RESOURCE_MANAGER_H
#include "common/ResourceManager.h"
#include "Renderer/Texture.h"
#include "Renderer/Mesh.h"
namespace Render {
	class Model : public IResource {
	public:
		const std::map<Name, TexturePtr>& getTextureBindings()const;
		const MeshPtr& getMesh()							  const;
	private:
		std::map<Name, TexturePtr>		mTextureBindings;
		MeshPtr							mMesh;
	};

	class ModelResourceManager : public ResourceManager<Model> {
	public:

	};
}
#endif