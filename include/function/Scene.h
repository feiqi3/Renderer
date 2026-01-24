#ifndef _SCENE_H_
#define _SCENE_H_

#include "function/ObjectFwd.h"
#include "common/Name.h"
#include "Renderer/LightManager.h"
#include <vector>
#include <memory>
#include <list>

namespace Render {
	class Camera;
	class Component;
	class Object;
    struct rs_drawdata;
    class Scene {
    public:
        Scene();
        ~Scene();

        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;

        Object* createObject(const char* name = nullptr);
        void destroyObject(Object* object);
        void destroyObjectByID(ObjectID id);
        const std::list<std::unique_ptr<Object>>& objects() const noexcept;
        LightManager& getLightMgr();
        const LightManager& getLightMgr()const;
        rs_drawdata* getSceneDrawData()const;
        void update(float deltaTime);
    private:
        friend class Object;

        uint32_t generateObjectID();

    private:
        rs_drawdata* m_drawData = nullptr;

        uint32_t m_nextObjectID = 1;
        std::list<std::unique_ptr<Object>> m_objects;
        std::unique_ptr<LightManager> m_lightMgr;
    };


}

#endif