#ifndef LIGHT_H_
#define LIGHT_H_

#include "common/CommonMath.h"
#include "Renderer/GPUShared/SceneData.h"
namespace Render {

    enum class LightType {
        Directional = 0,
        Point = 1,
        Spot = 2
    };

    class Light {
    public:
        Light(LightType type, const glm::vec3& color = vec3(1.0f), float intensity = 1.0f);
        virtual ~Light() = default;

        void setPosition(const glm::vec3& pos);
        void setDirection(const glm::vec3& dir);
        void setColor(const glm::vec3& col);
        void setIntensity(float i);
        void setRange(float r);
        void setSpotAngles(float inner, float outer);

        LightType getType() const;
        const glm::vec3& getPosition() const;
        const glm::vec3& getDirection() const;
        const glm::vec3& getColor() const;
        float getIntensity() const;
        float getRange() const;
        float getInnerCone() const;
        float getOuterCone() const;

        // Dirty flag control
        void setDirty(bool lightDataDirty = true);
        bool isDirty() const;

        // GPU data
        GPUShared::GPULightData toGPUData() const;

    private:
        LightType m_type;
        glm::vec3 m_position = glm::vec3(0.0f);
        glm::vec3 m_direction = glm::vec3(0.0f, -1.0f, 0.0f);
        glm::vec3 m_color = glm::vec3(1.0f);
        float m_intensity = 1.0f;
        float m_range = 10.0f;
        float m_innerCone = 20.0f;
        float m_outerCone = 30.0f;
        bool m_dirty = true;
    };

} // namespace Render

#endif // LIGHT_H_