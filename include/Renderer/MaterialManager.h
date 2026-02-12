#ifndef MATERIAL_RESOURCE_MANAGER_H_
#define MATERIAL_RESOURCE_MANAGER_H_

#include "common/Singleton.h"
#include "common/ResourceManager.h"
#include "common/Name.h"
#include "Renderer/MaterialInstance.h" 

namespace Render {

    class MaterialManager :
        public ResourceManager<Material>,
        public Singleton<MaterialManager>
    {
    public:
        MaterialManager();
        virtual ~MaterialManager();

        virtual const Name& typeName() const override;
        template <typename T, typename... Args>
        MaterialPtr createMaterial(const Name& materialName, Args&&... args)
        {
            static_assert(std::is_base_of<Material, T>::value, "T must inherit from Render::Material");

            auto* entry = this->acquire(materialName);
            if (entry) {
                assert(0 && "Error: duplicated material name");
                return nullptr;
            }

            T* mat = new T(std::forward<Args>(args)...);

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

			return ResourceSystem::instance()->getResource<Material>(Material::typeName(), materialName);
        }
    protected:
        virtual Material* loadImpl(const Name& id) override;
        virtual void unloadImpl(Material* res) override;

    private:
        virtual void createNecessaryPersistenceResources() override {}
    };
}

#endif