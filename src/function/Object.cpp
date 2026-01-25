#include "function/Object.h"
#include "common/CommonMath.h"
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

    ObjectID Object::id() const noexcept { return m_id; }
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

    void Object::updateTransformRecursive(bool needUpdate)
    {
        bool updateFlag = false;
        updateFlag = needUpdate || this->isTransformDirty();
        if (updateFlag == true) {
            updateWorldTransform();
            m_transformDirty = false;
        }
        for (auto& child : children()) {
            child->updateTransformRecursive(updateFlag);
        }
    }

    void Object::updateWorldTransform()
    {
        mat4 TRS = getTRS(m_localPosition, m_localRotation, m_localScale);
        if (parent()) {
            m_worldMatrix = parent()->worldMatrix() * TRS;

        }
        else {
            m_worldMatrix = TRS;
        }
        //vec4 point(0, 0, 0,1);
        //m_worldPosition = m_worldMatrix * point;
        //Or faster:
        // GLM column-major: translation in column 3
        m_worldPosition = vec3(m_worldMatrix[3][0], m_worldMatrix[3][1], m_worldMatrix[3][2]);
    }

    bool Object::parentCycleCheck(Object* target)
    {
        if (target == nullptr)return false;

        Object* par = target;

        do {
            if (par == this) {
                return true;
            }
            par = par->parent();
        } while (par != nullptr);
        return false;
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
        bool hasCycle = parentCycleCheck(parent);
        if (hasCycle) {
            assert(0);
            //TODO:  
            return;
        }
        
        if (m_parent) {
            auto& siblings = m_parent->m_children;
            siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
        }

        m_parent = parent;

        // attach to new parent
        if (m_parent) {
            m_parent->m_children.push_back(this);
        }
        markTransformDirty();
    }

    void Object::addChild(Object* child) {
        assert(child != nullptr);
        for (auto&& c : children()) {
            if (c == child)
            {
                //TODO:
                assert(0);
                return;
            }
        }
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