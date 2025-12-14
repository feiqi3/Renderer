#include "function/Component.h"
#include "function/Object.h"
namespace Render{
    bool Component::enabled() const noexcept
    {
        return m_enabled;
    }
    void Component::setEnabled(bool enabled)
    {
        if (this->enabled() == enabled)
        {
            return;
        }

        if (this->enabled() == false)
        {
            m_enabled = enabled;
            this->onEnable();
        }
        else {
            m_enabled = enabled;
            this->onDisable();
        }
    }

    Object* Component::owner() const noexcept
{
    return m_owner;
}
void Component::setOwner(Object* owner)
{
    m_owner = owner;
}
}
