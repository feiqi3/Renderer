#ifndef ANIMATION_H_
#define ANIMATION_H_
#pragma once
#include "common/CommonMath.h"
#include "common/name.h"
#include <vector>
namespace Render::Anm {


	struct Transform {
		quat		rotation;
		vec3		translation;
		vec3		scale;

		inline mat4 toMatrix() const;
	};

	template <class T>
	struct KeyFrame {
		T		value;
		double	time = 0.0;
	};

	using TranslationKey	= KeyFrame<vec3>;
	using RotationKey		= KeyFrame<quat>;
	using ScaleKey			= KeyFrame<vec3>;

	enum class Interpolation {
		Nearest,
		Linear,
//		Cubic,
		SphericalLinear,	//Slerp --> angle
	};

	struct Track {
		Name			mTargetName;			//which target this track will be imposed on
		Interpolation	mInterpolation;			//interpolation function
	};


	template <class T>
	T		_interpolate(T a, T b, float t, Interpolation type);

	quat	_interpolate(quat a, quat b, float t, Interpolation type);


	template<class T> struct KeyFrameTrack : Track {
		std::vector<T> mKeyData;
		inline bool isEmpty()const { return mKeyData.empty(); }
		using ValueType = decltype(std::declval<T>().value);

		//Sample Animation at time t
		inline ValueType sample(float t) const {
			if (mKeyData.empty()) {
				return ValueType();
			}

			auto itor = std::lower_bound(mKeyData.begin(), mKeyData.end(), t, [](const T& ele, float tTar ) {
				return ele.time < tTar;
			});
			if (itor == mKeyData.end()) {
				return mKeyData.back().value;
			}

			if (itor == mKeyData.begin()) {
				return mKeyData.begin()->value;
			}

			auto itort_1 = itor -1;
			const auto& frameLeft	= *itort_1;
			const auto& frameRight = *itor;
			float t0 = frameLeft.time;
			float t1 = frameRight.time;
			float dt = t1 - t0;
			float tt = (dt > 0.0f) ? (t - t0) / dt : 0.0f;
			return _interpolate(frameLeft.value, frameRight.value, tt, mInterpolation);
		}
	};

	
	using RotationTrack = KeyFrameTrack<RotationKey>;
	using TranslationTrack = KeyFrameTrack<TranslationKey>;
	using ScaleTrack = KeyFrameTrack<ScaleKey>;


	template <class T>
	inline T _interpolate(T a, T b, float t, Interpolation type)
	{
		switch (type) {
		case Interpolation::Nearest: {
			float tN = clamp(t, 0.f, 1.f);
			if (tN < 0.5)
				return a;
			return b;
		}break;
		case Interpolation::SphericalLinear:
			assert(false && "Not support interpolation");
		case Interpolation::Linear:

		default:
		{
			return a * (1. - t) + b * (t);
		}break;


		}
		return T();
	}

	class JointAnimation {
	private:
		Name mJointName;
		RotationTrack		mRotTrack;
		TranslationTrack	mTransTrack;
		ScaleTrack			mScaleTrack;

		Transform sample(float t);
	};


}


#endif//!ANIMATION_H_