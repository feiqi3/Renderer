#include "function/Sphere.h"
#include "function/AABB.h"
#include <cmath>
#include <algorithm>

namespace Render {

	Sphere::Sphere() : m_center(vec3(INFINITY)), m_radius(INFINITY)
	{
	}

	Sphere::Sphere(const vec3& center, float radius) : m_center(center), m_radius(radius)
	{
	}

	bool Sphere::isInfinity() const
	{
		return m_center.x == INFINITY || m_radius == INFINITY;
	}

	bool Sphere::contains(const vec3& point) const
	{
		if (this->isInfinity()) return false;

		float distSq = distance2(m_center, point);
		return distSq <= (m_radius * m_radius);
	}

	bool Sphere::intersects(const Sphere& other) const
	{
		if (this->isInfinity() || other.isInfinity()) return false;

		float distSq = distance2(m_center, other.m_center);
		float radiusSum = m_radius + other.m_radius;
		return distSq <= (radiusSum * radiusSum);
	}

	bool Sphere::intersects(const AxisAlignedBoundingBox& aabb) const
	{
		if (this->isInfinity() || aabb.isInfinity()) return false;

		vec3 closestPoint;
		closestPoint.x = std::max(aabb.getMin().x, std::min(m_center.x, aabb.getMax().x));
		closestPoint.y = std::max(aabb.getMin().y, std::min(m_center.y, aabb.getMax().y));
		closestPoint.z = std::max(aabb.getMin().z, std::min(m_center.z, aabb.getMax().z));

		return contains(closestPoint);
	}

	void Sphere::expand(const vec3& point)
	{
		if (this->isInfinity()) {
			m_center = point;
			m_radius = 0.0f;
			return;
		}

		float dist = distance(m_center, point);
		if (dist > m_radius) {
			float newRadius = (m_radius + dist) * 0.5f;
			vec3 toPoint = (point - m_center) / dist;
			m_center = m_center + toPoint * (newRadius - m_radius);
			m_radius = newRadius;
		}
	}

	void Sphere::expand(const Sphere& other)
	{
		if (other.isInfinity()) return;
		if (this->isInfinity()) {
			m_center = other.m_center;
			m_radius = other.m_radius;
			return;
		}

		float dist = distance(m_center, other.m_center);
		if (dist + other.m_radius <= m_radius) return;
		if (dist + m_radius <= other.m_radius) {
			m_center = other.m_center;
			m_radius = other.m_radius;
			return;
		}

		float newRadius = (m_radius + dist + other.m_radius) * 0.5f;
		vec3 toOther = (other.m_center - m_center) / dist;
		m_center = m_center + toOther * (newRadius - m_radius);
		m_radius = newRadius;
	}
}