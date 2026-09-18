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

		mat4 toMatrix() const;
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
	T		_interpolate(const T& a,const T& b, float t, Interpolation type);

	quat	_interpolate(const quat& a, const quat& b, float t, Interpolation type);


	template<typename KeyType>
	struct KeyFrameDefaultValue {
		static auto Get() {
			return decltype(KeyType::value){};
		}
	};

	template<class T, class DefaultValue =  KeyFrameDefaultValue<T>> struct KeyFrameTrack : Track {
		std::vector<T> mKeyData;
		inline bool isEmpty()const { return mKeyData.empty(); }
		using ValueType = decltype(std::declval<T>().value);

		inline ValueType _interpolateInner(uint32_t l, uint32_t r, float t) const{

			if (l == r) return mKeyData[l].value;
			const auto& left	= mKeyData[l];
			const auto& right	= mKeyData[r];

			float timeLeft		= left.time;
			float timeRight		= right.time;

			float timeStep = timeRight - timeLeft;
			float iF = (timeStep > 0.0f) ? (t - timeLeft) / timeStep : 0.0f;

			return _interpolate(left.value, right.value, iF, mInterpolation);

		}

		//Sample Animation at time t
		inline ValueType sample(float t, int32_t& lastIndex, bool isBackSearch = false) const {
			if (mKeyData.empty()) return DefaultValue::Get();

			auto itor = std::lower_bound(mKeyData.begin(), mKeyData.end(), t, [](const T& ele, float tTar ) {
				return ele.time < tTar;
			});
			if (itor == mKeyData.end()) {
				lastIndex = mKeyData.size() - 1;
				return mKeyData.back().value;
			}

			if (itor == mKeyData.begin()) {
				lastIndex = 0;
				return mKeyData.begin()->value;
			}

			auto itort_1 = itor -1;
			uint32_t left, right;
			left	= itort_1 - mKeyData.begin();
			right	= itor - mKeyData.begin();

			lastIndex = isBackSearch ? right : left;

			return _interpolateInner(left, right, t);
		}

		//Sample Animation at time t
		inline ValueType sampleFromLast(float t, int32_t& lastIndex, bool isBackSearch = false) const {
			if (lastIndex >= int32_t(mKeyData.size())) {
				assert(false);
				return DefaultValue::Get();
			}
			
			if (mKeyData.empty()) return DefaultValue::Get();

			//fallback to normal search

			if (lastIndex < 0) return sample(t, lastIndex, isBackSearch);

			if (isBackSearch) {
				if (t > mKeyData[lastIndex].time) return sample(t,lastIndex,isBackSearch);
			}
			else {
				if (t < mKeyData[lastIndex].time) return sample(t,lastIndex,isBackSearch);
			}

			auto anmBeginTime = mKeyData.front().time;

			if (t <= anmBeginTime || t >= mKeyData.back().time) {
				if (t <= anmBeginTime) {
					lastIndex = 0;
					return mKeyData.front().value;
				}
				else {
					lastIndex = static_cast<int32_t>(mKeyData.size() - 1);
					return mKeyData.back().value;
				}
			}
			uint32_t idxLeft = lastIndex;
			uint32_t idxRight = lastIndex;
			//Search from last index?   
			if (isBackSearch) {
				for (int i = lastIndex; i >= 0; --i) {
					auto tCur = mKeyData[i].time;
					if (t > tCur) {
						idxLeft = i;
						idxRight = std::min(i + 1, lastIndex);
						break;
					}
				}

			}
			else {
				for (int i = lastIndex; i < mKeyData.size(); ++i) {
					auto tCur = mKeyData[i].time;
					if (t < tCur) {
						idxLeft = std::max(i - 1,0);
						idxRight = i;
						break;
					}
				}
			}
			lastIndex = (int32_t)(isBackSearch ? idxRight : idxLeft);
			return _interpolateInner(idxLeft, idxRight, t);
		}
	};


	//Default value for scale
	struct KeyFrameDefaultValueScaleKey {
		static vec3 Get() {
			return vec3(1.0f, 1.0f, 1.0f);
		}
	};
	
	using RotationTrack		= KeyFrameTrack<RotationKey>;
	using TranslationTrack	= KeyFrameTrack<TranslationKey>;
	using ScaleTrack		= KeyFrameTrack<ScaleKey,KeyFrameDefaultValueScaleKey>;


	template <class T>
	inline T _interpolate(const T& a, const T& b, float t, Interpolation type)
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

	struct JointAnimation {
		Name mJointName;
		RotationTrack		mRotTrack;
		TranslationTrack	mTransTrack;
		ScaleTrack			mScaleTrack;

	};

	struct SkeletonAnimation {
		Name						mSkeletonAnimationName;
		std::vector<JointAnimation> mJointAnimations;
		float						mDuration = 0.;
		float						mSampleRate = 30.; //x samples per frame
	};

}


#endif//!ANIMATION_H_