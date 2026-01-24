#include "Renderer/Camera.h"

#include "Renderer/RenderSystem.h"                 
#include "Renderer/GPUShared/CameraData.h"         
#include "common/CommonMath.h"                     

namespace Render {

    Camera::Camera(const Name& name,
        const vec3& position,
        const vec3& target,
        const vec3& up,
        float fovDegrees,
        float aspectRatio,
        float nearPlane,
        float farPlane)
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

    Camera::~Camera()
    {
        if (mCameraDrawData) {
            RenderSystem::instance()->destroyDrawData(mCameraDrawData);
            mCameraDrawData = nullptr;
        }
    }

    void Camera::updateView()
    {
        m_view = lookAt(m_position, m_target, m_up);
    }

    void Camera::updateProjection()
    {
        m_projection = perspective(radians(m_fov), m_aspect, m_near, m_far);
    }

    void Camera::setPosition(const vec3& pos)
    {
        m_position = pos;
        updateView();
    }

    void Camera::setTarget(const vec3& tgt)
    {
        m_target = normalize(tgt);
        updateView();
    }

    void Camera::setUp(const vec3& up)
    {
        m_up = normalize(up);
        updateView();
    }

    void Camera::setPerspective(float fovDegrees, float aspect, float nearPlane, float farPlane)
    {
        m_fov = fovDegrees;
        m_aspect = aspect;
        m_near = nearPlane;
        m_far = farPlane;
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

        // fill matrices
        common.MatView = getViewMatrix();
        common.MatProj = getProjectionMatrix();

        // MatViewProj: projection * view (typical column-major convention)
        common.MatViewProj = common.MatProj * common.MatView;

        // inverses
        common.MatInvView = inverse(common.MatView);
        common.MatInvProj = inverse(common.MatProj);

        // vectors
        common.CameraPosition = vec4(getPosition(), 1.0f);
        common.CameraUp = vec4(getUp(), 0.0f);

        // Camera front: normalized (target - position) is usually the forward direction
        common.CameraFront = vec4(normalize(getTarget() - getPosition()), 0.0f);

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
