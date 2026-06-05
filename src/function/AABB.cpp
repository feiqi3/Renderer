#include "function/AABB.h"
#include <cmath>

namespace Render {
	AxisAlignedBoundingBox::AxisAlignedBoundingBox() : minPoint(vec3(INFINITY)), maxPoint(vec3(INFINITY))
	{
	}
	AxisAlignedBoundingBox::AxisAlignedBoundingBox(const vec3& min, const vec3& max) :minPoint(min), maxPoint(max)
	{
	}
	bool AxisAlignedBoundingBox::isInfinity() const
	{
		return minPoint.x == INFINITY;
	}
	vec3 AxisAlignedBoundingBox::getCenter() const
	{
		if (this->isInfinity())return vec3(INFINITY);
		return minPoint + (maxPoint - minPoint) * 0.5f;
	}
	vec3 AxisAlignedBoundingBox::getSize() const
	{
		if (this->isInfinity())return vec3(INFINITY);
		return maxPoint - minPoint;
	}

	AxisAlignedBoundingBox AxisAlignedBoundingBox::transform(const mat4& trans)
	{
		if (this->isInfinity()) return AxisAlignedBoundingBox();

		vec3 corners[8] = {
			vec3(minPoint.x, minPoint.y, minPoint.z),
			vec3(maxPoint.x, minPoint.y, minPoint.z),
			vec3(minPoint.x, maxPoint.y, minPoint.z),
			vec3(maxPoint.x, maxPoint.y, minPoint.z),
			vec3(minPoint.x, minPoint.y, maxPoint.z),
			vec3(maxPoint.x, minPoint.y, maxPoint.z),
			vec3(minPoint.x, maxPoint.y, maxPoint.z),
			vec3(maxPoint.x, maxPoint.y, maxPoint.z)
		};

		AxisAlignedBoundingBox transformedBox;

		for (int i = 0; i < 8; ++i) {
			vec4 homogeneousCorner(corners[i].x, corners[i].y, corners[i].z, 1.0f);
			vec4 transformedHomogeneous = trans * homogeneousCorner;
			vec3 transformedCorner(transformedHomogeneous.x, transformedHomogeneous.y, transformedHomogeneous.z);

			transformedBox.expand(transformedCorner);
		}

		return transformedBox;
	}

	bool AxisAlignedBoundingBox::intersects(const AxisAlignedBoundingBox& other) const
	{
		if (this->isInfinity())return false;
		return (minPoint.x <= other.maxPoint.x && maxPoint.x >= other.minPoint.x) &&
			(minPoint.y <= other.maxPoint.y && maxPoint.y >= other.minPoint.y) &&
			(minPoint.z <= other.maxPoint.z && maxPoint.z >= other.minPoint.z);
	}
	bool AxisAlignedBoundingBox::contains(const vec3& point) const
	{
		if (this->isInfinity())return false;

		return (point.x >= minPoint.x && point.x <= maxPoint.x) &&
			(point.y >= minPoint.y && point.y <= maxPoint.y) &&
			(point.z >= minPoint.z && point.z <= maxPoint.z);
	}
	void AxisAlignedBoundingBox::expand(const vec3& point)
	{
		if (this->isInfinity()) {
			this->minPoint = point;
			this->maxPoint = point;
		}

		minPoint.x = std::min(minPoint.x, point.x);
		minPoint.y = std::min(minPoint.y, point.y);
		minPoint.z = std::min(minPoint.z, point.z);

		maxPoint.x = std::max(maxPoint.x, point.x);
		maxPoint.y = std::max(maxPoint.y, point.y);
		maxPoint.z = std::max(maxPoint.z, point.z);
	}
	void AxisAlignedBoundingBox::expand(const AxisAlignedBoundingBox& other)
	{
		expand(other.minPoint);
		expand(other.maxPoint);
	}
}