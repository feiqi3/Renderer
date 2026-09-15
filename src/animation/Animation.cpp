#include "animation/animation.h"
namespace Render::Anm {

	mat4 Transform::toMatrix() const
	{
		return translate(MAT4IDENTITY, translation)
			* mat4_cast(rotation)
			* ::Render::scale(MAT4IDENTITY, scale);
	}

	quat _interpolate(const quat& a, const quat& b, float t, Interpolation type)
	{
		switch (type) {
		case Interpolation::Nearest: {
			float tN = clamp(t, 0.f, 1.f);
			if (tN < 0.5)
				return a;
			return b;
		}break;
		case Interpolation::Linear:
		{
			auto quatB = b;
			if (dot(a, b) < 0) {
				quatB = -b;
			}
			vec4 aa(a.x, a.y, a.z, a.w);
			vec4 bb(quatB.x, quatB.y, quatB.z, quatB.w);
			auto ret = _interpolate(aa, bb, t, Interpolation::Linear);
			quat quatRet(ret.w, ret.x, ret.y, ret.z);
			return normalize(quatRet);
		}break;

		case Interpolation::SphericalLinear:
		default:
		{
			return slerp(a,b,t);
		}

		}
		return quat();
	}
}
