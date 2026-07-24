#include "function/Scene.h"
#include "function/Object.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/Camera.h"
#include "Components/RenderComponent.h"
#include "Renderer/RenderEntity.h"
#include "Renderer/MaterialInstance.h"
#include "function/CullUtils.h"
#include <algorithm>
#include <cassert>
#include "Renderer/DebugDrawManager.h"
#include "function/InputManager.h"
namespace Render {

	static Scene* s_currentScene = nullptr;

    void Scene::setCurrentScene(Scene* scene)
    {
		s_currentScene = scene;
    }

    Scene* Scene::getCurrentScene()
    {
        return s_currentScene;
    }


    Scene::Scene() {
        m_lightMgr = std::make_unique<LightManager>();
        m_shadowMgr = std::make_unique<ShadowManager>();
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


        if (objIdxInVec != m_objects.size() - 1) {
            objectToSwapId = m_objects.back()->id();
            std::swap(m_objects[objIdxInVec], m_objects.back());
			m_objectIdx[objectToSwapId] = objIdxInVec;
		}
        m_objects.pop_back();
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

	Render::ShadowManager& Scene::getShadowMgr()
	{
        return *m_shadowMgr;
	}

	const Render::ShadowManager& Scene::getShadowMgr() const
	{
		return *m_shadowMgr;
	}

	rs_drawdata* Scene::getSceneDrawData() const
    {
        return m_drawData;
    }

	Render::rs_drawdata* Scene::getSceneShadowData() const
	{
        return m_shadowMgr->getShadowDrawData();
	}

	void Scene::update(float deltaTime) {
        _doDelayDestroy();
        for (auto& obj : m_objects) {
            Object* o = obj.get();
            if (!o) continue;

            o->onUpdate(deltaTime);
        }

		updateObjectsTransform();

		for (auto& obj : m_objects) {
            Object* o = obj.get();
            if (!o) continue;

            o->onLateUpdate(deltaTime);
        }
    }

    void Scene::renderOneFrame()
    {
    }

	void Scene::registerRenderComponent(RenderComponent* comp)
	{
		if (!comp || comp->getSceneIndex() != static_cast<size_t>(-1)) return;

		m_renderComponents.push_back(comp);

		comp->setSceneIndex(m_renderComponents.size() - 1);
	}

	void Scene::unregisterRenderComponent(RenderComponent* comp)
	{
		if (!comp || comp->getSceneIndex() == static_cast<size_t>(-1)) return;

		size_t targetIdx = comp->getSceneIndex();
		size_t lastIdx = m_renderComponents.size() - 1;

		if (targetIdx != lastIdx) {
			std::swap(m_renderComponents[targetIdx], m_renderComponents[lastIdx]);

			m_renderComponents[targetIdx]->setSceneIndex(targetIdx);
		}

		m_renderComponents.pop_back();

		comp->setSceneIndex(static_cast<size_t>(-1));
	}

	void Scene::collectVisibleObjects(Camera* camera, CustomCullFunction cull)
	{
        if (!camera) return;
        m_entities.clear();
		float viewportNearZ;float viewportFarZ;
		RenderSystem::instance()->getGlobalViewportZRange(viewportNearZ, viewportFarZ);

        static bool shouldForzeCull = false;
        if (InputManager::instance()->isKeyDown(KeyCode::F)) {
            shouldForzeCull = !shouldForzeCull;
        }

        if (!shouldForzeCull) {
			camFrustum.update(*camera, viewportNearZ, viewportFarZ);
        }

        auto renderQueue = camera->getRenderQueue();
        renderQueue->clear();
        auto camType = camera->getType();
        bool isShadowCam = camType == CameraType::Shadow;
        uint32_t cullMask = camera->getCullMask();
       
        for (auto&& comp : m_renderComponents) {
            if (!comp) continue;
            if (comp->enabled() == false)continue;
            if (isShadowCam && !comp->isCastShadow()) {
				continue;
			}

			if ((cullMask & comp->getCullMask()) == 0) {
				continue;
			}
			comp->collectRenderEntities(m_entities);

        }

		for (RenderEntity* entity : m_entities)
		{
			if (entity && entity->getMaterial())
			{
                auto aabb = entity->getWorldBounding();
                bool isVisible = false;
                if (cull != nullptr) {
                    isVisible = cull(camFrustum,aabb);
                }
                else {
                    isVisible = camFrustum.isVisible(aabb);
                }
				if (!isVisible)continue;
				u64 renderMask = entity->getMaterial()->getRenderMask();
				renderQueue->submit(entity, renderMask);
			}
		}
    }

	const std::vector<class RenderEntity*>& Scene::getCollectedEntites() const
	{
        return m_entities;
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