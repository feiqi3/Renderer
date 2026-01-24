#include "Renderer/Light.h"
#include <cmath>

namespace Render {

    Light::Light(LightType type, const vec3& color, float intensity)
        : m_type(type),
        m_color(color),
        m_intensity(intensity)
    {
        m_dirty = true;
    }

    void Light::setPosition(const vec3& pos) {
        m_position = pos;
        setDirty();
    }

    void Light::setDirection(const vec3& dir) {
        m_direction = normalize(dir);
        setDirty();
    }

    void Light::setColor(const vec3& col) {
        m_color = col;
        setDirty();
    }

    void Light::setIntensity(float i) {
        m_intensity = i;
        setDirty();
    }

    void Light::setRange(float r) {
        m_range = r;
        setDirty();
    }

    void Light::setSpotAngles(float inner, float outer) {
        m_innerCone = inner;
        m_outerCone = outer;
        setDirty();
    }

    // Getters
    LightType Light::getType() const {
        return m_type;
    }

    const vec3& Light::getPosition() const {
        return m_position;
    }

    const vec3& Light::getDirection() const {
        return m_direction;
    }

    const vec3& Light::getColor() const {
        return m_color;
    }

    float Light::getIntensity() const {
        return m_intensity;
    }

    float Light::getRange() const {
        return m_range;
    }

    float Light::getInnerCone() const {
        return m_innerCone;
    }

    float Light::getOuterCone() const {
        return m_outerCone;
    }

    // Dirty flag
    void Light::setDirty(bool dirty) {
        m_dirty = dirty;
    }

    bool Light::isDirty() const {
        return m_dirty;
    }

    // 构造 GPU 数据
    GPUShared::GPULightData Light::toGPUData() const {
        GPUShared::GPULightData data{};

        // 1. Position + Type
        data.positionType = vec4(m_position, static_cast<float>(m_type));

        // 2. Direction + Range
        data.directionRange = vec4(m_direction, m_range);

        // 3. Color + Intensity
        data.colorIntensity = vec4(m_color, m_intensity);

        float innerCos = std::cos(radians(m_innerCone));
        float outerCos = std::cos(radians(m_outerCone));
        data.spotParams = vec4(innerCos, outerCos, 0.0f, 0.0f);

        return data;
    }

} // namespace Render