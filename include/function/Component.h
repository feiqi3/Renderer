#ifndef COMPONENT_H_
#define COMPONENT_H_

#include "function/ObjectFwd.h"
namespace Render {
    class Scene;

    class Component {
    public:
        virtual ~Component() = default;

        virtual void onAttach() {}

        virtual void onDetach() {}

        virtual void onEnable() {}

        virtual void onDisable() {}

        virtual void onUpdate(float dt) {}

        virtual void onDestroy() {};

        virtual void onFrameEnd() {};
        bool enabled() const noexcept;
        void setEnabled(bool enabled);

        Object* owner() const noexcept;

    protected:
        Component() = default;

    private:
        friend class Object;
        void setOwner(Object* owner);

        Object* m_owner = nullptr;
        bool m_enabled = true;

        friend class ComponentSystem;
    };
};

#endif