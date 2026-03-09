#ifndef _OBJECT_H_
#define _OBJECT_H_

#include "function/ObjectFwd.h"
#include "function/ComponentSystem.h"
#include "function/ComponentFwd.h"
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

		template<typename T>
		int hasComponentsNum() const;

		template<typename T>
		T* getComponent(int idx) const;

		template<typename T>
		T* getComponents(int idx) const;

		template<typename T>
        void removeComponent();

		template<typename T>
		bool removeComponent(int idx);

		template<typename T>
		int removeComponents();

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
        void destroyAllComponent();
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
        std::vector<ComponentUniquePtr> m_components;
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

	template<typename T>
	int Object::hasComponentsNum() const
	{
        int num = 0;
		for (const auto& c : m_components) {
			if (auto ptr = dynamic_cast<T*>(c.get())) {
                num++;
			}
		}
        return num;
	}

    template<typename T>
    inline T* Object::getComponent(int idx) const
    {
		int i = 0;
		for (const auto& c : m_components) {
			if (auto ptr = dynamic_cast<T*>(c.get())) {
                if (i == idx) {
                    return ptr;
                }
				i++;
			}
		}
		return nullptr;
    }

    template<typename T>
    inline T* Object::getComponents(int idx) const
    {
        std::vector<T*> components;
		for (const auto& c : m_components) {
			if (auto ptr = dynamic_cast<T*>(c.get())) {
                components.push_back(ptr);
			}
		}
		return components;
    }

	template<typename T>
	void Object::removeComponent()
	{
        removeComponent<T>(0);
	}

	template<typename T>
    bool Object::removeComponent(int idx)
    {
		bool begRemove = false;

		int targetCmptCnt = 0;

        for (int i = 1;i < m_components.size();++i) {
            const auto& cmpt = m_components[i - 1];
            if (!begRemove && dynamic_cast<T*>(cmpt.get()) != nullptr) {
                targetCmptCnt++;
                if (targetCmptCnt == idx) {
                    begRemove = true;
                }
            }
            std::swap(m_components[i - 1], m_components[i]);
        }

        if (!begRemove) {
            if (dynamic_cast<T*>(m_components.back().get()) != nullptr) {
                begRemove = true;
            }
        }

        if (begRemove) {
            ComponentSystem::instance()->delegateDestroyComponent(m_components.back());
            m_components.resize(m_components.size() - 1);
            return true;
        }
        else {
            return false;
        }
    }

	template<typename T>
	int Object::removeComponents()
	{
        int removeCnt = 0;
        auto removeItor = std::remove_if(
            m_components.begin(), m_components.end(), [](const auto& cmpnt) {
                if (dynamic_cast<T*>(cmpnt.get()) != nullptr) {
                    removeCnt++;
                    return true;
                }
                else {
                    return false;
                }
            }
        );
		auto cmpntSys = ComponentSystem::instance();
        for (auto itor = removeItor;itor != m_components.end();++itor) {
            cmpntSys->delegateDestroyComponent(*itor);
        }
		m_components.erase(removeItor, m_components.end());
        return removeCnt;
	}

	inline std::span<const std::unique_ptr<Component>> Object::components() const noexcept {
        return { m_components.data(), m_components.size() };
    }

}

#endif