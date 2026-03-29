#include "Renderer/ModelResourceManager.h"
#include "Renderer/GltfLoader.h"
#include "function/Scene.h"
#include "function/Object.h"
#include "Components/PBRRenderComponent.h"
namespace Render {
    Render::SamplerDesc fromGltfSamplerToSamplerDesc(const Render::GLTFSampler& sampler) {
        using namespace Render;

        Render::SamplerDesc desc{};
        desc.addressU = sampler.addressS;
        desc.addressV = sampler.addressT;
        desc.addressW = AddressMode::Repeat;
        desc.minFilter = sampler.minFilter;
        desc.magFilter = sampler.magFilter;
        return desc;
    }

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
         auto   model     = mGLTFLoader->gltfModelToEngimeModel(gltfModel);
         delete gltfModel;
         gltfModel = 0;

         return model;
    }
    void ModelResourceManager::unloadImpl(Model* model)
    {
    }
    const Name& Model::getTypeName() const
    {
        return Model::typeName();
    }
    Model::Model()
    {
    }
    Model::~Model()
    {
    }
    const Name& Model::typeName() {
        const static Name typeName = Name("Model");
        return typeName;
    }

    void Model::OnUnload() {
        mModelParts.clear();
        mState = ResourceLoadState::Unloaded;
    }

    ResourceMemory Model::getMemory() const {
        ResourceMemory mem{ 0, 0 };
        mem.cpuMemory = (u32)sizeof(*this);

        mem.cpuMemory += (u32)(mModelParts.capacity() * sizeof(ModelPart));
        for (const auto& part : mModelParts) {
            mem.cpuMemory += (u32)(part.materials.capacity() * sizeof(MaterialPtr));
        }

        return mem;
    }

    // ================== Getters ==================

    const std::vector<Model::ModelPart>& Model::getModelParts() const {
        return mModelParts;
    }

    std::vector<Model::ModelPart>& Model::getModelParts() {
        return mModelParts;
    }

    void Model::addMesh(const MeshPtr& mesh) {
        ModelPart part;
        part.mesh = mesh;
        if (mesh) {
            part.materials.resize(mesh->getSubMeshCount());
        }
        mModelParts.push_back(part);
    }

    void Model::addMesh(const MeshPtr& mesh, const std::vector<MaterialPtr>& materials) {
        ModelPart part;
        part.mesh = mesh;
        part.materials = materials;
		assert(mesh != nullptr && materials.size() == mesh->getSubMeshCount());
        mModelParts.push_back(part);
    }

    void Model::setMaterial(size_t partIndex, size_t subMeshIndex, const MaterialPtr& mat) {
        if (partIndex >= mModelParts.size()) {
            return; 
        }

        auto& part = mModelParts[partIndex];

        if (subMeshIndex >= part.materials.size()) {
            if (part.mesh && subMeshIndex < part.mesh->getSubMeshCount()) {
                part.materials.resize(subMeshIndex + 1);
            }
            else {
                part.materials.resize(subMeshIndex + 1);
            }
        }

        part.materials[subMeshIndex] = mat;
    }
    Object* Model::toSceneNode(Scene* scene, const Name& nodeName)
    {
        Object* obj = scene->createObject(nodeName.c_str());
        assert(obj != nullptr);
        for (size_t i = 0; i < mModelParts.size(); ++i) {
            const auto& part = mModelParts[i];
            if (!part.mesh) continue;
            auto renderComp = obj->addComponent<PBRRenderComponent>();
            renderComp->setMesh(part.mesh);
            for (size_t j = 0; j < part.materials.size(); ++j) {
                renderComp->setMaterial(j, part.materials[j]);
            }
        }
		return obj;
    }
}