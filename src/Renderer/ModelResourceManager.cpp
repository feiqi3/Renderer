#include "Renderer/ModelResourceManager.h"
#include "tiny_gltf.h"


namespace Render {
    const Name& Render::ModelResourceManager::typeName() const
    {
        Model::typeName();
    }
    Model* ModelResourceManager::loadImpl(const Name& id)
    {
        return nullptr;
    }
    const std::map<Name, TexturePtr>& Model::getTextureBindings() const
    {
        return mTextureBindings;
    }
    const MeshPtr& Model::getMesh() const
    {
        return mMesh;
    }
    const Name& Model::getTypeName() const
    {
        return Model::typeName();
    }
    const Name& Model::typeName() {
        const static Name typeName = Name("Model");
        return typeName;
    }
}