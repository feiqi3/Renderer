#ifndef CULL_UTILS_H_
#define CULL_UTILS_H_
#include <array>

#include "common/CoreDefs.h"
#include "function/AABB.h"
#include "function/Plane.h"
namespace Render {
	class Camera;
	class Sphere;
	class AxisAlignedBoundingBox;

	class Frustum {
	public:
		enum PlaneIndex {
			Left = 0,
			Right,
			Bottom,
			Top,
			Near,
			Far,
			Count
		};

		Frustum() = default;

		void update(const Camera& camera);

		bool isVisible(const vec3& point) const;
		bool isVisible(const Sphere& sphere) const;
		bool isVisible(const AxisAlignedBoundingBox& aabb) const;

		inline const Plane& getPlane(PlaneIndex index) const { return m_planes[index]; }

	private:
		std::array<Plane, PlaneIndex::Count> m_planes;
	};
};

#endif

