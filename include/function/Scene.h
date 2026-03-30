#ifndef _SCENE_H_
#define _SCENE_H_

#include "function/ObjectFwd.h"
#include "common/Name.h"
#include "Renderer/LightManager.h"
#include <vector>
#include <memory>
#include <map>
#include <mutex>
namespace Render {
	class Camera;
	class Component;
	class Object;
    struct rs_drawdata;
    class Scene {
    public:
		static void setCurrentScene(Scene* scene);
		static Scene* getCurrentScene();

        Scene();
        ~Scene();

        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;

        void destroyObjectByID(ObjectID id);
        void destroyObject(Object* object);

        Object* createObject(const char* name = nullptr);
        Object* getObjectById(ObjectID id);
        LightManager& getLightMgr();
        const LightManager& getLightMgr()const;
        rs_drawdata* getSceneDrawData()const;
        void update(float deltaTime);

        virtual void renderOneFrame();
    private:
        void updateObjectsTransform();

        friend class Object;

        void postDestroyObject(Object* object);

        uint32_t generateObjectID();
        void eraseAndChangeObjIndex(ObjectID idToDestroy,size_t posToDestroy);

        void _doDelayDestroy();
        void _destroyObjectByID(ObjectID id);
        void _destroyObject(Object* object);
    private:

        std::vector<Object*> m_pendingDestroyObjects;
        std::vector<ObjectID> m_pendingDestroyObjectsID;

        std::unique_ptr<LightManager> m_lightMgr;

        rs_drawdata* m_drawData = nullptr;
        uint32_t m_nextObjectID = 1;
        std::vector<std::unique_ptr<Object>> m_objects;
        std::map<ObjectID, size_t> m_objectIdx;
        std::mutex m_mutex;
    };


}

#endif