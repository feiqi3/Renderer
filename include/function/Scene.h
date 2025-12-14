#ifndef _SCENE_H_
#define _SCENE_H_

#include "function/ObjectFwd.h"
#include "common/Name.h"
#include <vector>
#include <memory>
#include <list>

namespace Render {
	class Camera;
	class Component;
	class Object;

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

        void update(float deltaTime);
    private:
        friend class Object;

        uint32_t generateObjectID();

    private:
        uint32_t m_nextObjectID = 1;
        std::list<std::unique_ptr<Object>> m_objects;
    };


}

#endif