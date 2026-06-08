#include "function/CullUtils.h"
#include "Renderer/Camera.h"
#include "function/Sphere.h"
#include "function/AABB.h"

namespace Render {

	void Frustum::update(const Camera& camera,float viewportNearZ,float viewportFarZ)
	{
		mat4 vp = camera.getProjectionMatrix() * camera.getViewMatrix();

		const static vec4 viewportCorner[8] = {
			vec4(-1	,1	,viewportNearZ	,1.)		,	//LB
			vec4(1	,1	,viewportNearZ	,1.)		,	//RB
			vec4(1	,-1	,viewportNearZ	,1.)		,	//RT
			vec4(-1	,-1	,viewportNearZ	,1.)		,	//LT
			vec4(-1	,1	,viewportFarZ	,1.)		,
			vec4(1	,1	,viewportFarZ	,1.)		,
			vec4(1	,-1	,viewportFarZ	,1.)		,
			vec4(-1	,-1	,viewportFarZ	,1.)		,
		};
		mat4 invVp = inverse(vp);
		vec4 frustumCorner[8];
		
		//Get frustum corner points in world space.
		for (int i = 0;i < 8; ++i) {
			frustumCorner[i] = invVp * viewportCorner[i];
			frustumCorner[i] /= frustumCorner[i].w;
		}

		// Left Plane
		m_planes[Left]		= Plane(frustumCorner[4], frustumCorner[3], frustumCorner[0]);

		// Right Plane
		m_planes[Right]		= Plane(frustumCorner[2], frustumCorner[1], frustumCorner[5]);

		// Bottom Plane
		m_planes[Bottom]	= Plane(frustumCorner[1], frustumCorner[0], frustumCorner[4]);

		// Top Plane
		m_planes[Top]		= Plane(frustumCorner[7], frustumCorner[3], frustumCorner[2]);

		// Near Plane
		m_planes[Near]		= Plane(frustumCorner[0], frustumCorner[1], frustumCorner[2]);

		// Far Plane
		m_planes[Far]		= Plane(frustumCorner[6], frustumCorner[5], frustumCorner[4]);

		for (auto& plane : m_planes) {
			plane.normalizePlane();
		}
	}

	bool Frustum::isVisible(const vec3& point) const
	{
		for (const auto& plane : m_planes) {
			if (plane.intersects(point) < 0.) return false;
		}
		return true;
	}

	bool Frustum::isVisible(const Sphere& sphere) const
	{
		for (const auto& plane : m_planes) {
			if (plane.intersects(sphere) < 0.) return false;
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