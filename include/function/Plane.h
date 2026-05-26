#ifndef PLANE_H
#define PLANE_H

#include "common/CommonMath.h"

namespace Render {

	class AxisAlignedBoundingBox;
	class Sphere;

	class Plane {
	private:
		vec3 m_normal;
		float m_distance;

	public:
		Plane();
		Plane(const vec3& normal, const vec3& pointOnPlane);
		Plane(const vec3& normal, float distance);
		//counter clock wise!!!! --> normal related
		Plane(const vec3& p0, const vec3& p1, const vec3& p2);

		inline const vec3& getNormal() const { return m_normal; }
		inline float getDistance() const { return m_distance; }

		void normalizePlane();

		float getSignedDistance(const vec3& point) const;

		//False if in in the negative direction of normal
		bool intersects(const vec3& point) const;
		bool intersects(const Sphere& sphere) const;
		bool intersects(const vec3& center,float radius) const;
		bool intersects(const AxisAlignedBoundingBox& aabb) const;
	};
}

#endif // PLANE_H