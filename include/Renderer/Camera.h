#pragma once
#ifndef CAMERA_H_
#define CAMERA_H_

#include "common/CommonMath.h"
#include "common/Name.h"

namespace Render {
	struct rs_drawdata;
	class CameraManager;

	namespace GPUShared { struct GPUCameraData; } // forward-declare GPU data struct

	enum class CameraType {
		Perspective,
		Orthographic
	};

	class Camera {
	public:
		Camera(const Name& name,
			const vec3& position = vec3(0.0f, 0.0f, 3.0f),
			const vec3& target = vec3(0.0f, 0.0f, 0.0f),
			const vec3& up = vec3(0.0f, 1.0f, 0.0f),
			float fovDegrees = 45.0f,
			float aspectRatio = 16.0f / 9.0f,
			float nearPlane = 0.1f,
			float farPlane = 100.0f);

		Camera(const Name& name,
			const vec3& position,
			const vec3& target,
			const vec3& up,
			float orthoSize,
			float aspectRatio,
			float nearPlane,
			float farPlane);

		~Camera();

		// ========== Getters ==========
		inline const mat4& getViewMatrix() const { return m_view; }
		inline const mat4& getProjectionMatrix() const { return m_projection; }
		inline const vec3& getPosition() const { return m_position; }
		inline const vec3& getDirection() const { return m_direction; }
		inline const vec3& getUp() const { return m_up; }
		inline const float getFar() const { return m_far; }
		inline const float getNear() const { return m_near; }
		inline const Name& getName() const { return mCamName; }

		inline CameraType getType() const { return m_type; }
		inline float getOrthoSize() const { return m_orthoSize; }
		inline float getAspectRatio() const { return m_aspect; }
		inline float getFov() const { return m_fov; }

		// ========== Setters ==========
		void setPosition(const vec3& pos);
		void setTarget(const vec3& tgt);
		void setDirection(const vec3& dir);
		void setUp(const vec3& up);

		void setPerspective(float fovDegrees, float aspect, float nearPlane, float farPlane);
		void setOrthographic(float orthoSize, float aspect, float nearPlane, float farPlane);

		void setOrthographicBounds(float left, float right, float bottom, float top);

		void setType(CameraType type);
		void setOrthoSize(float size);
		void setAspectRatio(float aspect);
		void setFar(float far);
		void setNear(float near);

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

		CameraType m_type;
		float m_fov;         // Perspective
		float m_orthoSize;   // Orthographic
		float m_aspect;      
		float m_near;
		float m_far;

		vec2 m_orthoCenterOffset = vec2(0.0f, 0.0f);

		mat4 m_view;
		mat4 m_projection;

		bool m_active = true;

		friend class CameraManager;
		rs_drawdata* mCameraDrawData = nullptr;
	};

} // namespace Render

#endif // CAMERA_H_