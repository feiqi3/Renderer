#ifndef MATERIAL_RESOURCE_MANAGER_H_
#define MATERIAL_RESOURCE_MANAGER_H_

#include "common/Name.h"
#include "common/ResourceManager.h"
#include "Renderer/MaterialInstance.h" 
#include "common/ResourceSystem.h"
namespace Render {

    class MaterialManager :
        public ResourceManager<Material>,
        public Singleton<MaterialManager>
    {
    public:
        MaterialManager();
        virtual ~MaterialManager();

        virtual const Name& typeName() const override;
        inline MaterialPtr getMaterial(const Name& materialName) {
			auto* entry = this->acquire(materialName);
            if (entry == nullptr)return nullptr;
			return ResourceHandle<Material>(this, entry);

        }

        template <typename T, typename... Args>
        inline MaterialPtr createMaterial(const Name& materialName, Args&&... args)
        {
            static_assert(std::is_base_of<Material, T>::value, "T must inherit from Render::Material");

            auto* entry = this->acquire(materialName);
            if (entry) {
                assert(0 && "Error: duplicated material name");
                return nullptr;
            }

            T* mat = new T(std::forward<Args>(args)...);
            mat->mMaterialIndex = ++mGlobalMaterialIndex;
            auto* newEntry = this->registerResource(
                materialName,
                mat,
                ResourceLifetime::Transient,
                nullptr
            );

            if (!newEntry) {
                delete mat;
                return nullptr;
            }
            return ResourceHandle<Material>(this, newEntry);
        }
    protected:
        virtual Material* loadImpl(const Name& id) override;
        virtual void unloadImpl(Material* res) override;

    private:
        virtual void createNecessaryPersistenceResources() override;
        uint32_t mGlobalMaterialIndex = 0;
    };
}

#endif