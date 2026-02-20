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
        struct ModelPart {
            MeshPtr mesh;
            std::vector<MaterialPtr> materials; 
        };

    public:
        Model();
        virtual ~Model();

        static const Name& typeName();
        virtual const Name& getTypeName() const override;
        virtual ResourceMemory getMemory() const override;
        virtual void OnUnload() override;

        // Getters
        const std::vector<ModelPart>& getModelParts() const;
        std::vector<ModelPart>& getModelParts();

        void addMesh(const MeshPtr& mesh);
        void addMesh(const MeshPtr& mesh, const std::vector<MaterialPtr>& materials);

        void setMaterial(size_t partIndex, size_t subMeshIndex, const MaterialPtr& mat);

        class Object* toSceneNode(class Scene* scene, const Name& nodeName);
    private:
        std::vector<ModelPart>      mModelParts;
    };


	class ModelResourceManager : public ResourceManager<Model>{
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