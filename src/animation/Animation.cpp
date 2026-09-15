#include "animation/animation.h"
namespace Render::Anm {

	mat4 Transform::toMatrix() const
	{
		return translate(MAT4IDENTITY, translation)
			* mat4_cast(rotation)
			* ::Render::scale(MAT4IDENTITY, scale);
	}

	quat _interpolate(quat a, quat b, float t, Interpolation type)
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
			//Not right, but available
			vec4 aa(a.x, a.y, a.z, a.w);
			vec4 bb(b.x, b.y, b.z, b.w);
			auto ret = _interpolate(aa, bb, t, Interpolation::Linear);
			quat quatRet(ret.w, ret.x, ret.y, ret.z);
			return quatRet;
		}break;

		case Interpolation::SphericalLinear:
		default:
		{
			return slerp(a,b,t);
		}

		}
		return quat();
	}

	Transform JointAnimation::sample(float t)
	{
		//sample three track    
		Transform ret{};
		ret.translation = mTransTrack.sample(t);
		ret.scale		= mScaleTrack.sample(t);
		ret.rotation	= mRotTrack.sample(t);
		return ret;
	}
}
