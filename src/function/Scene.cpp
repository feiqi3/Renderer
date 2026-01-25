#include "function/Scene.h"
#include "function/Object.h"
#include "Renderer/RenderSystem.h"
#include <algorithm>
#include <cassert>

namespace Render {

    Scene::Scene() {
        m_lightMgr = std::make_unique<LightManager>();
        m_drawData = RenderSystem::instance()->createDrawData();
    }

    Scene::~Scene() {
        for (auto& obj : m_objects) {
            Object* o = obj.get();
            if (!o) continue;

            o->onDeactivate();
            o->onExitScene(this);
            o->onDestroy();
        }
        m_objects.clear();
        RenderSystem::instance()->destroyDrawData(m_drawData);
    }

    void Scene::postDestroyObject(Object* object)
    {
        object->onDeactivate();
        object->onExitScene(this);
        object->onDestroy();
    }

    uint32_t Scene::generateObjectID() {
        return m_nextObjectID++;
    }

    void Scene::destroyObjectByID(ObjectID id)
    {
        this->m_pendingDestroyObjectsID.push_back(id);
    }

    void Scene::destroyObject(Object* object)
    {
        this->m_pendingDestroyObjects.push_back(object);
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

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_objects.emplace_back(std::move(obj));
            m_objectIdx[rawPtr->id()] = m_objects.size() - 1;
        }
        return rawPtr;
    }

    Object* Scene::getObjectById(ObjectID id)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto itor = m_objectIdx.find(id);
            if (itor == m_objectIdx.end()) {
                return nullptr;
            }

            auto index = itor->second;
            return m_objects[index].get();
        }
    }

    void Scene::_destroyObject(Object* object) {
        if (!object) return;
        assert(object->scene() == this && "Object does not belong to this Scene");

        std::unique_ptr<Object> toDestroy = nullptr;
        ObjectID toDestroyId = INVALID_ID;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto itor = m_objectIdx.find(object->id());
            if (itor == m_objectIdx.end()) {
                return;
            }
            size_t posToDestroy = itor->second;

            toDestroy = std::move(m_objects[posToDestroy]);
            toDestroyId = toDestroy->id();
            eraseAndChangeObjIndex(toDestroyId, posToDestroy);
        }
        postDestroyObject(toDestroy.get());
    }

    void Scene::eraseAndChangeObjIndex(ObjectID id, size_t objIdxInVec)
    {
        if (m_objects.empty() || objIdxInVec >= m_objects.size()) return;
        ObjectID objectToSwapId = INVALID_ID;
        objectToSwapId = m_objects.back()->id();

        std::swap(m_objects[objIdxInVec], m_objects.back());

        m_objects.resize(m_objects.size() - 1);


        m_objectIdx[objectToSwapId] = objIdxInVec;
        m_objectIdx.erase(id);
    }

    void Scene::_doDelayDestroy()
    {
        for (auto&& obj : m_pendingDestroyObjects) {
            _destroyObject(obj);
        }
        m_pendingDestroyObjects.clear();
        for (auto&& ID : m_pendingDestroyObjectsID) {
            _destroyObjectByID(ID);
        }
        m_pendingDestroyObjectsID.clear();
    }

    void Scene::_destroyObjectByID(ObjectID id)
    {
        std::unique_ptr<Object> toDestroy = nullptr;
        ObjectID toDestroyId = INVALID_ID;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            
            auto itor = m_objectIdx.find(id);
            if (itor == m_objectIdx.end()) {
                return;
            }
            size_t posToDestroy = itor->second;

            toDestroy = std::move(m_objects[posToDestroy]);
            toDestroyId = toDestroy->id();

            eraseAndChangeObjIndex(toDestroyId, posToDestroy);
        }
        postDestroyObject(toDestroy.get());

    }

    LightManager& Scene::getLightMgr()
    {
        return *m_lightMgr;
    }

    const LightManager& Scene::getLightMgr() const
    {
        return *m_lightMgr;
    }

    rs_drawdata* Scene::getSceneDrawData() const
    {
        return m_drawData;
    }

    void Scene::update(float deltaTime) {
        _doDelayDestroy();
        updateObjectsTransform();

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

    void Scene::updateObjectsTransform()
    {
        for (auto& obj : m_objects) {
            if (obj->isRoot()) {
                obj->updateTransformRecursive(false);
            }
        }
    }

} // namespace Render