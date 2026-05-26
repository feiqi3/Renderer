#include "function/CullUtils.h"
#include "Renderer/Camera.h"
#include "function/Sphere.h"
#include "function/AABB.h"

namespace Render {

	void Frustum::update(const Camera& camera)
	{
		mat4 vp = camera.getProjectionMatrix() * camera.getViewMatrix();


		// Left Plane
		m_planes[Left] = Plane(
			vec3(vp[0][3] + vp[0][0], vp[1][3] + vp[1][0], vp[2][3] + vp[2][0]),
			(vp[3][3] + vp[3][0])
		);

		// Right Plane
		m_planes[Right] = Plane(
			vec3(vp[0][3] - vp[0][0], vp[1][3] - vp[1][0], vp[2][3] - vp[2][0]),
			(vp[3][3] - vp[3][0])
		);

		// Bottom Plane
		m_planes[Bottom] = Plane(
			vec3(vp[0][3] + vp[0][1], vp[1][3] + vp[1][1], vp[2][3] + vp[2][1]),
			(vp[3][3] + vp[3][1])
		);

		// Top Plane
		m_planes[Top] = Plane(
			vec3(vp[0][3] - vp[0][1], vp[1][3] - vp[1][1], vp[2][3] - vp[2][1]),
			(vp[3][3] - vp[3][1])
		);

		// Near Plane
		m_planes[Near] = Plane(
			vec3(vp[0][3] + vp[0][2], vp[1][3] + vp[1][2], vp[2][3] + vp[2][2]),
			(vp[3][3] + vp[3][2])
		);

		// Far Plane
		m_planes[Far] = Plane(
			vec3(vp[0][3] - vp[0][2], vp[1][3] - vp[1][2], vp[2][3] - vp[2][2]),
			(vp[3][3] - vp[3][2])
		);

		for (auto& plane : m_planes) {
			plane.normalizePlane();
		}
	}

	bool Frustum::isVisible(const vec3& point) const
	{
		for (const auto& plane : m_planes) {
			if (!plane.intersects(point)) return false;
		}
		return true;
	}

	bool Frustum::isVisible(const Sphere& sphere) const
	{
		for (const auto& plane : m_planes) {
			if (!plane.intersects(sphere)) return false;
		}
		return true;
	}

	bool Frustum::isVisible(const AxisAlignedBoundingBox& aabb) const
	{
		if (aabb.isInfinity())return true;

		for (const auto& plane : m_planes) {
			if (!plane.intersects(aabb)) return false;
		}
		return true;
	}

} // namespace Render