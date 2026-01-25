#ifndef _OBJECT_H_
#define _OBJECT_H_

#include "function/ObjectFwd.h"
#include "common/CommonMath.h"
#include "common/Name.h"
#include "function/Component.h"
#include <memory>
#include <span>
namespace Render {
    class Component;

    struct ObjectCommon {
        mat4 WorldTransform;         
        mat4 WorldTransformInv;       
        mat4 NormalTransform; 
    };

    class Object {
    public:

        // ctor/dtor
        explicit Object(ObjectID id, std::string name = "");
        virtual ~Object();

        Object(const Object&) = delete;
        Object& operator=(const Object&) = delete;

        // identity
        ObjectID id() const noexcept;
        const std::string& name() const noexcept;
        void setName(const std::string& name) noexcept;

        // scene
        Scene* scene() const noexcept;

        const vec3& localPosition() const noexcept;
        const quat& localRotation() const noexcept;
        const vec3& localScale() const noexcept;

        void setLocalPosition(const vec3& pos);
        void setLocalRotation(const quat& rot);
        void setLocalScale(const vec3& scale);

        const mat4& worldMatrix() const noexcept;
        const vec3& worldPosition() const noexcept;

        void markTransformDirty() noexcept;
        bool isTransformDirty() const noexcept;

        // -------------------------
        // Hierarchy
        // -------------------------
        Object* parent() const noexcept;
        std::span<Object* const> children() const noexcept;

        void setParent(Object* parent);
        void addChild(Object* child);
        void removeChild(Object* child);

        bool isRoot() const noexcept;


        template<typename T, typename... Args>
        T* addComponent(Args&&... args);

        template<typename T>
        T* getComponent() const;

        template<typename T>
        bool hasComponent() const;

        std::span<const std::unique_ptr<Component>> components() const noexcept;

        virtual void onCreate() {}
        virtual void onDestroy() {}

        virtual void onEnterScene(Scene* scene) {}
        virtual void onExitScene(Scene* scene) {}

        virtual void onActivate();
        virtual void onDeactivate();

        virtual void onUpdate(float dt);
        virtual void onLateUpdate(float dt);

    protected:
        void setScene(Scene* scene) noexcept;
        void updateTransformRecursive(bool needUpdate);
        void updateWorldTransform();
    private:
        bool parentCycleCheck(Object* target);

        friend class Scene;

        ObjectID m_id;
        std::string m_name;

        Scene* m_scene = nullptr;

        vec3 m_localPosition{ 0.f };
        quat m_localRotation{ 1.f, 0.f, 0.f, 0.f };
        vec3 m_localScale{ 1.f, 1.f, 1.f };

        mat4 m_worldMatrix{ 1.f };
        vec3 m_worldPosition{ 0.f };
        bool m_transformDirty = true;

        // hierarchy
        Object* m_parent = nullptr;
        std::vector<Object*> m_children;

        // components ownership
        std::vector<std::unique_ptr<Component>> m_components;
    };


    template<typename T, typename... Args>
    T* Object::addComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

        auto comp = std::make_unique<T>(std::forward<Args>(args)...);

        comp->setOwner(this);

        comp->onAttach();
        if (comp->enabled()) {
            comp->onEnable();
        }

        T* ptr = comp.get();
        m_components.push_back(std::move(comp));
        return ptr;
    }

    template<typename T>
    T* Object::getComponent() const {
        for (const auto& c : m_components) {
            if (auto ptr = dynamic_cast<T*>(c.get())) {
                return ptr;
            }
        }
        return nullptr;
    }

    template<typename T>
    bool Object::hasComponent() const {
        return getComponent<T>() != nullptr;
    }

    inline std::span<const std::unique_ptr<Component>> Object::components() const noexcept {
        return { m_components.data(), m_components.size() };
    }

}

#endif