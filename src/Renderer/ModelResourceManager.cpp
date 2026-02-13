#include "Renderer/ModelResourceManager.h"
#include "Renderer/GltfLoader.h"
namespace Render {
    ModelResourceManager::ModelResourceManager()
    {
		mGLTFLoader = new GLTFLoader();
    }
    ModelResourceManager::~ModelResourceManager()
    {
        delete mGLTFLoader;
        mGLTFLoader = nullptr;
    }
    const Name& Render::ModelResourceManager::typeName() const
    {
        return Model::typeName();
    }
    Model* ModelResourceManager::loadImpl(const Name& id)
    {
        //GLTF Model
         auto   gltfModel = mGLTFLoader->createFromFilePath(id.str());
         Model* model = new Model();
         //1. generate materials;
    }
    const std::map<Name, TexturePtr>& Model::getTextureBindings() const
    {
        return mTextureBindings;
    }
    const std::vector<MeshPtr>& Model::getMeshs() const
    {
        return mMeshs;
    }
    std::map<Name, TexturePtr>& Model::getTextureBindings()
    {
        return mTextureBindings;
    }
    std::vector<MeshPtr>& Model::getMeshs()
    {
        return mMeshs;
    }
    const Name& Model::getTypeName() const
    {
        return Model::typeName();
    }
    const Name& Model::typeName() {
        const static Name typeName = Name("Model");
        return typeName;
    }
    ResourceMemory Model::getMemory() const
    {
        ResourceMemory memory{};
        memory.cpuMemory = sizeof(*this);
        return memory;
    }
}