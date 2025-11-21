#pragma once
#ifndef CAMERA_H_
#define CAMERA_H_

#include "common/CommonMath.h"
#include "common/Name.h"
namespace Render {
    class CameraManager;
    class Camera{
        public:
        Camera(const Name& name,
            const vec3& position = vec3(0.0f, 0.0f, 3.0f),
            const vec3& target = vec3(0.0f, 0.0f, 0.0f),
            const vec3& up = vec3(0.0f, 1.0f, 0.0f),
            float fovDegrees = 45.0f,
            float aspectRatio = 16.0f / 9.0f,
            float nearPlane = 0.1f,
            float farPlane = 100.0f
        )
            : mCamName(name)
            , m_position(position)
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
        const mat4& getViewMatrix() const { return m_view; }
        const mat4& getProjectionMatrix() const { return m_projection; }
        const vec3& getPosition() const { return m_position; }
        const vec3& getTarget() const { return m_target; }
		const vec3& getUp() const {	return m_up;}
        // ========== Setters ==========
        void setPosition(const vec3& pos) {
            m_position = pos;
            updateView();
        }

        void setTarget(const vec3& tgt) {
            m_target = normalize(tgt);
            updateView();
        }

        void setUp(const vec3& up) {
            m_up = normalize(up);
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
        void setViewMatrix(const mat4& view) { m_view = view; }
        void setProjectionMatrix(const mat4& proj) { m_projection = proj; }
        const Name& getName() const { return mCamName; }
		~Camera();
    private:
        void updateView() {
            m_view = lookAt(m_position, m_target, m_up);
        }

        void updateProjection() {
            m_projection = perspective(radians(m_fov), m_aspect, m_near, m_far);
        }
    
    private:
        Name mCamName;

        vec3 m_position;
        vec3 m_target;
        vec3 m_up;

        float m_fov;
        float m_aspect;
        float m_near;
        float m_far;

        mat4 m_view;
        mat4 m_projection;

        friend class CameraManager;
		rs_drawdata* mCameraDrawData = nullptr;
	};

	Camera::~Camera()
	{
        if (mCameraDrawData) {
            RenderSystem::instance()->destroyDrawData(mCameraDrawData);
            mCameraDrawData = 0;
        }
	}

}

#endif