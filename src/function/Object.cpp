#include "function/Object.h"

#include <algorithm>
#include <cassert>

namespace Render {

    // -------------------------
    // ctor / dtor
    // -------------------------

    Object::Object(ObjectID id, std::string name)
        : m_id(id), m_name(std::move(name)) {
    }

    Object::~Object() {
        for (auto* child : m_children) {
            if (child) child->m_parent = nullptr;
        }
        m_children.clear();

        for (auto& comp : m_components) {
            if (!comp) continue;
            if (comp->enabled()) {
                comp->onDisable();
            }
            comp->onDetach();
        }
        m_components.clear();
    }

    Object::ObjectID Object::id() const noexcept { return m_id; }
    const std::string& Object::name() const noexcept { return m_name; }
    void Object::setName(const std::string& name) noexcept { m_name = name; }

    Scene* Object::scene() const noexcept { return m_scene; }

    void Object::onActivate()
    {
        for (auto&& comp : m_components) {
            comp.get()->setEnabled(true);
        }
    }

    void Object::onDeactivate()
    {
        for (auto&& comp : m_components) {
            comp.get()->setEnabled(false);
        }
    }

    void Object::onUpdate(float dt)
    {
        for (auto& comp : m_components) {
            comp.get()->onUpdate(dt);
        }
    }

    void Object::onLateUpdate(float dt)
    {
    }

    void Object::setScene(Scene* scene) noexcept {
        m_scene = scene;
    }

    const vec3& Object::localPosition() const noexcept { return m_localPosition; }
    const quat& Object::localRotation() const noexcept { return m_localRotation; }
    const vec3& Object::localScale() const noexcept { return m_localScale; }

    void Object::setLocalPosition(const vec3& pos) {
        m_localPosition = pos;
        markTransformDirty();
    }

    void Object::setLocalRotation(const quat& rot) {
        m_localRotation = rot;
        markTransformDirty();
    }

    void Object::setLocalScale(const vec3& scale) {
        m_localScale = scale;
        markTransformDirty();
    }

    const mat4& Object::worldMatrix() const noexcept { return m_worldMatrix; }
    const vec3& Object::worldPosition() const noexcept { return m_worldPosition; }

    void Object::markTransformDirty() noexcept {
        if (!m_transformDirty) {
            m_transformDirty = true;
        }
    }

    bool Object::isTransformDirty() const noexcept {
        return m_transformDirty;
    }

    Object* Object::parent() const noexcept { return m_parent; }

    std::span<Object* const> Object::children() const noexcept {
        return { m_children.data(), m_children.size() };
    }

    void Object::setParent(Object* parent) {
        if (m_parent == parent) return;

        if (m_parent) {
            auto& siblings = m_parent->m_children;
            siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
        }

        m_parent = parent;

        // attach to new parent
        if (m_parent) {
            m_parent->m_children.push_back(this);
            markTransformDirty();
        }
    }

    void Object::addChild(Object* child) {
        assert(child != nullptr);
        child->setParent(this);
    }

    void Object::removeChild(Object* child) {
        if (!child) return;
        if (child->m_parent == this) {
            child->setParent(nullptr);
        }
    }

    bool Object::isRoot() const noexcept {
        return m_parent == nullptr;
    }


}