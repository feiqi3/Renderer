#ifndef MODEL_RESOURCE_MANAGER_H
#define MODEL_RESOURCE_MANAGER_H
#include "common/ResourceManager.h"
#include "Renderer/Texture.h"
#include "Renderer/Mesh.h"
#include "common/CommonMath.h"
namespace Render {

	class Model : public IResource {
	public:
		const std::map<Name, TexturePtr>& getTextureBindings()const;
		const MeshPtr& getMesh()							  const;
		virtual const Name& getTypeName() const override;
		static const Name& typeName();
	private:
		std::map<Name, TexturePtr>		mTextureBindings;
		MeshPtr							mMesh;
	};

	class ModelResourceManager : public ResourceManager<Model> {
	public:
		ModelResourceManager() = default;
		virtual const Name& typeName() const override;
		virtual Model* loadImpl(const Name& id)override;
		virtual void unloadImpl(Model* model)override;
	private:
	};
}
#endif