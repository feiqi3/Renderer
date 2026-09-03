#include "Renderer/Camera.h"

#include "Renderer/RenderSystem.h"                 
#include "Renderer/GPUShared/CameraData.h"         
#include "common/CommonMath.h"                     

namespace Render {

	Camera::Camera(const Name& name,
		const vec3& position,
		const vec3& target,
		const vec3& up,
		float orthoSize,
		float aspectRatio,
		float nearPlane,
		float farPlane)
		: mCamName(name)
		, m_position(position)
		, m_direction(target - position)
		, m_up(up)
		, m_projType(CameraProjectType::Perspective)
		, m_fov(45.0f) 
		, m_orthoSize(orthoSize)
		, m_aspect(aspectRatio)
		, m_near(nearPlane)
		, m_far(farPlane)
	{
		updateView();
		updateProjection();
	}

	Camera::~Camera()
	{
		if (mCameraDrawData) {
			RenderSystem::instance()->destroyDrawData(mCameraDrawData);
			mCameraDrawData = nullptr;
		}
	}

	void Camera::updateView()
	{
		m_view = lookAt(m_position, m_position + m_direction, m_up);
	}

	void Camera::updateProjection()
	{
		if (m_projType == CameraProjectType::Perspective)
		{
			m_projection = perspective(radians(m_fov), m_aspect, m_near, m_far);
		}
		else // CameraProjectType::Orthographic
		{
			float top = m_orthoSize;
			float bottom = -m_orthoSize;
			float right = m_orthoSize * m_aspect;
			float left = -right;

			left += m_orthoCenterOffset.x;
			right += m_orthoCenterOffset.x;
			bottom += m_orthoCenterOffset.y;
			top += m_orthoCenterOffset.y;

			m_projection = ortho(left, right, bottom, top, m_near, m_far);
		}
	}

	void Camera::setPosition(const vec3& pos)
	{
		m_position = pos;
		updateView();
	}

	void Camera::setTarget(const vec3& tgt)
	{
		m_direction = normalize(tgt - m_position);
		updateView();
	}

	void Camera::setDirection(const vec3& dir)
	{
		m_direction = normalize(dir);
		updateView();
	}

	void Camera::setUp(const vec3& up)
	{
		m_up = normalize(up);
		updateView();
	}

	void Camera::setPerspective(float fovDegrees, float aspect, float nearPlane, float farPlane)
	{
		m_projType = CameraProjectType::Perspective;
		m_fov = fovDegrees;
		m_aspect = aspect;
		m_near = nearPlane;
		m_far = farPlane;
		updateProjection();
	}

	void Camera::setOrthographic(float orthoSize, float aspect, float nearPlane, float farPlane)
	{
		m_projType = CameraProjectType::Orthographic;
		m_orthoSize = orthoSize;
		m_aspect = aspect;
		m_near = nearPlane;
		m_far = farPlane;
		m_orthoCenterOffset = vec2(0.0f, 0.0f); 
		updateProjection();
	}

	void Camera::setOrthographicBounds(float left, float right, float bottom, float top)
	{
		m_projType = CameraProjectType::Orthographic;

		float width = right - left;
		float height = top - bottom;

		m_orthoSize = height * 0.5f;

		m_aspect = (height != 0.0f) ? (width / height) : 1.0f;

		m_orthoCenterOffset.x = (left + right) * 0.5f;
		m_orthoCenterOffset.y = (bottom + top) * 0.5f;

		updateProjection();
	}

	void Camera::setProjType(CameraProjectType type)
	{
		if (m_projType != type) {
			m_projType = type;
			updateProjection();
		}
	}

	void Camera::setCamType(CameraType camType)
	{
		m_camType = camType;
	}

	void Camera::setCullMask(u32 cullMask)
	{
		mCullMask = cullMask;
	}

	Render::u32 Camera::getCullMask()
	{
		return mCullMask;
	}

	void Camera::setOrthoSize(float size)
	{
		m_orthoSize = size;
		if (m_projType == CameraProjectType::Orthographic) {
			updateProjection();
		}
	}

	void Camera::setAspectRatio(float aspect)
	{
		m_aspect = aspect;
		updateProjection();
	}

	void Camera::setFar(float far)
	{
		m_far = far;
		updateProjection();
	}

	void Camera::setNear(float near)
	{
		m_near = near;
		updateProjection();
	}

	void Camera::setViewMatrix(const mat4& view)
	{
		m_view = view;
	}

	void Camera::setProjectionMatrix(const mat4& proj)
	{
		m_projection = proj;
	}

	GPUShared::GPUCameraData Camera::toGPUData() const
	{
		GPUShared::GPUCameraData common{};
		common.MatView = getViewMatrix();
		common.MatProj = getProjectionMatrix();
		common.MatViewProj = common.MatProj * common.MatView;

		common.MatInvView = inverse(common.MatView);
		common.MatInvProj = inverse(common.MatProj);

		common.CameraPosition = vec4(getPosition(), 1.0f);
		common.CameraUp = vec4(getUp(), 0.0f);


		common.CameraFront = vec4(normalize(getDirection()), 0.0f);

		return common;
	}

	rs_drawdata* Camera::getDrawData()
	{
		if (!this->mCameraDrawData) {
			mCameraDrawData = RenderSystem::instance()->createDrawData();
		}
		return mCameraDrawData;
	}

} // namespace Render