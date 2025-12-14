#include "function/Scene.h"
#include "function/Object.h"

#include <algorithm>
#include <cassert>

namespace Render {

    Scene::Scene() = default;

    Scene::~Scene() {
        for (auto& obj : m_objects) {
            Object* o = obj.get();
            if (!o) continue;

            o->onDeactivate();
            o->onExitScene(this);
            o->onDestroy();
        }
        m_objects.clear();
    }

    uint32_t Scene::generateObjectID() {
        return m_nextObjectID++;
    }

    Object* Scene::createObject(const char* name) {
        auto obj = std::make_unique<Object>(
            generateObjectID(),
            name ? name : ""
        );

        Object* rawPtr = obj.get();

        rawPtr->onCreate();
        rawPtr->setScene(this);
        rawPtr->onEnterScene(this);
        rawPtr->onActivate(); 

        m_objects.emplace_back(std::move(obj));
        return rawPtr;
    }

    void Scene::destroyObject(Object* object) {
        if (!object) return;
        assert(object->scene() == this && "Object does not belong to this Scene");

        auto it = std::find_if(
            m_objects.begin(),
            m_objects.end(),
            [&](const std::unique_ptr<Object>& o) {
                return o.get() == object;
            }
        );

        if (it == m_objects.end())
            return;

        object->onDeactivate();
        object->onExitScene(this);
        object->onDestroy();

        m_objects.erase(it);
    }

    const std::list<std::unique_ptr<Object>>& Scene::objects() const noexcept {
        return m_objects;
    }

    void Scene::update(float deltaTime) {
        for (auto& obj : m_objects) {
            Object* o = obj.get();
            if (!o) continue;

            o->onUpdate(deltaTime);
        }

        for (auto& obj : m_objects) {
            Object* o = obj.get();
            if (!o) continue;

            o->onLateUpdate(deltaTime);
        }
    }

} // namespace Render