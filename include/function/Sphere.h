#ifndef SPHERE_H
#define SPHERE_H

#include "common/CommonMath.h"

namespace Render {

	class AxisAlignedBoundingBox;

	class Sphere {
	private:
		vec3 m_center;
		float m_radius;

	public:
		Sphere();
		Sphere(const vec3& center, float radius);

		inline const vec3& getCenter() const { return m_center; }
		inline float getRadius() const { return m_radius; }
		bool isInfinity() const;

		inline void setCenter(const vec3& center) { m_center = center; }
		inline void setRadius(float radius) { m_radius = radius; }

		bool contains(const vec3& point) const;
		bool intersects(const Sphere& other) const;
		bool intersects(const AxisAlignedBoundingBox& aabb) const;

		void expand(const vec3& point);
		void expand(const Sphere& other);
	};
}

#endif // SPHERE_H