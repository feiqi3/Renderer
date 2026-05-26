#include "function/Plane.h"
#include "function/AABB.h"
#include "function/Sphere.h"
#include <cmath>

namespace Render {

	Plane::Plane() : m_normal(vec3(0.0f, 1.0f, 0.0f)), m_distance(0.0f)
	{
	}

	Plane::Plane(const vec3& normal, const vec3& pointOnPlane)
	{
		m_normal = normalize(normal);
		m_distance = -dot(m_normal, pointOnPlane);
	}

	Plane::Plane(const vec3& normal, float distance) : m_normal(normal), m_distance(distance)
	{
	}

	Plane::Plane(const vec3& p0, const vec3& p1, const vec3& p2)
	{
		vec3 edge1 = p1 - p0;
		vec3 edge2 = p2 - p0;
		m_normal = normalize(cross(edge1, edge2));
		m_distance = -dot(m_normal, p0);
	}

	void Plane::normalizePlane()
	{
		float length = glm::length(m_normal);
		if (length > 0.0f) {
			m_normal /= length;
			m_distance /= length;
		}
	}

	float Plane::getSignedDistance(const vec3& point) const
	{
		return dot(m_normal, point) + m_distance;
	}

	bool Plane::intersects(const vec3& point) const
	{
		return getSignedDistance(point) >= 0.0f;
	}

	bool Plane::intersects(const Sphere& sphere) const
	{
		if (sphere.isInfinity()) return false;

		return intersects(sphere.getCenter(),sphere.getRadius());
	}

	bool Plane::intersects(const vec3& center, float radius) const {
		return getSignedDistance(center) >= -radius;
	}


	bool Plane::intersects(const AxisAlignedBoundingBox& aabb) const
	{
		if (aabb.isInfinity()) return false;

		vec3 halfExtents = aabb.getSize() * 0.5f;
		vec3 center = aabb.getCenter();

		//Project its long edge to normal
		float r = dot(halfExtents, m_normal);

		float sDist = getSignedDistance(center);

		return sDist >= -r;
	}
}