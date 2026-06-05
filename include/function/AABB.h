#ifndef AABB_H
#define AABB_H

#include "common/CommonMath.h"
namespace Render {
    class AxisAlignedBoundingBox {
    private:
        vec3 minPoint;
        vec3 maxPoint;
    public:
        AxisAlignedBoundingBox();

        AxisAlignedBoundingBox(const vec3& min, const vec3& max);

        inline const vec3& getMin() const { return minPoint; }
        inline const vec3& getMax() const { return maxPoint; }
        bool isInfinity() const;
        vec3 getCenter() const;
        vec3 getSize() const;

        AxisAlignedBoundingBox transform(const mat4& trans);

        bool intersects(const AxisAlignedBoundingBox& other) const;
        bool contains(const vec3& point) const;  

        void expand(const vec3& point);          
        void expand(const AxisAlignedBoundingBox& other);
    };
}
#endif // AABB_H