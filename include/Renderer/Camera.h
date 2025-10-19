#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
namespace Render {
    class Camera
    {
    public:
        Camera(
            const glm::vec3& position = glm::vec3(0.0f, 0.0f, 3.0f),
            const glm::vec3& target = glm::vec3(0.0f, 0.0f, 0.0f),
            const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f),
            float fovDegrees = 45.0f,
            float aspectRatio = 16.0f / 9.0f,
            float nearPlane = 0.1f,
            float farPlane = 100.0f
        )
            : m_position(position)
            , m_target(target)
            , m_up(up)
            , m_fov(fovDegrees)
            , m_aspect(aspectRatio)
            , m_near(nearPlane)
            , m_far(farPlane)
        {
            updateView();
            updateProjection();
        }

        // ========== Getters ==========
        const glm::mat4& getViewMatrix() const { return m_view; }
        const glm::mat4& getProjectionMatrix() const { return m_projection; }
        const glm::vec3& getPosition() const { return m_position; }
        const glm::vec3& getTarget() const { return m_target; }

        // ========== Setters ==========
        void setPosition(const glm::vec3& pos) {
            m_position = pos;
            updateView();
        }

        void setTarget(const glm::vec3& tgt) {
            m_target = tgt;
            updateView();
        }

        void setUp(const glm::vec3& up) {
            m_up = up;
            updateView();
        }

        void setPerspective(float fovDegrees, float aspect, float nearPlane, float farPlane) {
            m_fov = fovDegrees;
            m_aspect = aspect;
            m_near = nearPlane;
            m_far = farPlane;
            updateProjection();
        }

        // 直接设置矩阵（如果你想自己传入）
        void setViewMatrix(const glm::mat4& view) { m_view = view; }
        void setProjectionMatrix(const glm::mat4& proj) { m_projection = proj; }

    private:
        void updateView() {
            m_view = glm::lookAt(m_position, m_target, m_up);
        }

        void updateProjection() {
            m_projection = glm::perspective(glm::radians(m_fov), m_aspect, m_near, m_far);
        }

    private:
        glm::vec3 m_position;
        glm::vec3 m_target;
        glm::vec3 m_up;

        float m_fov;
        float m_aspect;
        float m_near;
        float m_far;

        glm::mat4 m_view;
        glm::mat4 m_projection;
    };
}