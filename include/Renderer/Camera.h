#pragma once
#ifndef CAMERA_H_
#define CAMERA_H_

#include "common/CommonMath.h"
#include "common/Name.h"

namespace Render {

    // 前向声明（减少头依赖）
    struct rs_drawdata;
    class CameraManager;

    namespace GPUShared { struct GPUCameraData; } // forward-declare GPU data struct

    class Camera {
    public:
        // ctor / dtor
        Camera(const Name& name,
            const vec3& position = vec3(0.0f, 0.0f, 3.0f),
            const vec3& target = vec3(0.0f, 0.0f, 0.0f),
            const vec3& up = vec3(0.0f, 1.0f, 0.0f),
            float fovDegrees = 45.0f,
            float aspectRatio = 16.0f / 9.0f,
            float nearPlane = 0.1f,
            float farPlane = 100.0f);

        ~Camera();

        // ========== Getters ==========
        inline const mat4& getViewMatrix() const { return m_view; }
        inline const mat4& getProjectionMatrix() const { return m_projection; }
        inline const vec3& getPosition() const { return m_position; }
        inline const vec3& getDirection() const { return m_direction; }
        inline const vec3& getUp() const { return m_up; }
        inline const Name& getName() const { return mCamName; }

        // ========== Setters ==========
        void setPosition(const vec3& pos);
        void setTarget(const vec3& tgt);
		void setDirection(const vec3& dir);
		void setUp(const vec3& up);
        void setPerspective(float fovDegrees, float aspect, float nearPlane, float farPlane);

        // 直接设置矩阵（如果你想自己传入）
        void setViewMatrix(const mat4& view);
        void setProjectionMatrix(const mat4& proj);

        // Convert to GPU shared layout (implementation in cpp)
        GPUShared::GPUCameraData toGPUData() const;

        bool getCameraActive() const { return m_active; }
        struct rs_drawdata* getDrawData();

    private:
        void updateView();
        void updateProjection();

    private:
        Name mCamName;

        vec3 m_position;
        vec3 m_direction;
        vec3 m_up;

        float m_fov;
        float m_aspect;
        float m_near;
        float m_far;

        mat4 m_view;
        mat4 m_projection;

        bool m_active = true;

        friend class CameraManager;
        rs_drawdata* mCameraDrawData = nullptr;
    };

} // namespace Render

#endif // CAMERA_H_