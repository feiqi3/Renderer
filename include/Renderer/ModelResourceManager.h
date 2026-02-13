#ifndef MODEL_RESOURCE_MANAGER_H
#define MODEL_RESOURCE_MANAGER_H
#include "common/ResourceManager.h"
#include "Renderer/Texture.h"
#include "Renderer/Mesh.h"
#include "common/CommonMath.h"
#include "Renderer/MaterialInstance.h"
namespace Render {

	class Model : public IResource {
	public:
		const std::map<Name, TexturePtr>& getTextureBindings()const;
		const std::vector<MeshPtr>& getMeshs()				const;
		std::vector<MaterialPtr>& getMaterials() { return mMaterials; }
		std::map<Name, TexturePtr>& getTextureBindings();
		std::vector<MeshPtr>& getMeshs()				;
		void  addMesh(const MeshPtr& mesh) { mMeshs.push_back(mesh); }
		void  addMaterial(const MaterialPtr& mat) { mMaterials.push_back(mat); }

		virtual const Name& getTypeName() const override;
		static const Name& typeName();
	
		virtual ResourceMemory getMemory() const override;

	private:
		std::map<Name, TexturePtr>		mTextureBindings;
		std::vector<MeshPtr>			mMeshs;
		std::vector<MaterialPtr>        mMaterials;
	};

	class ModelResourceManager : public ResourceManager<Model> {
	public:
		ModelResourceManager();
		~ModelResourceManager();
		virtual const Name& typeName() const override;
		virtual Model* loadImpl(const Name& id)override;
		virtual void unloadImpl(Model* model)override;
	private:
		class GLTFLoader* mGLTFLoader;
	};
}
#endif