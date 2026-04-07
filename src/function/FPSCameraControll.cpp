#include "function/FPSCameraControll.h"
namespace Render
{

	FPSCameraControl::FPSCameraControl()
	{
		mSensitivity = 0.5;
		mPitch = 0, mYaw = 0;
		mLocation = vec3(0,0,0);
		mCursorDx = 0, mCursorDy = 0;
	}


	void FPSCameraControl::setPitch(float rad)
	{
		markDirty();
		mPitch = rad;
	}

	void FPSCameraControl::setYaw(float rad)
	{
		markDirty();
		mYaw = rad;
	}

	float FPSCameraControl::getPitch() const
	{
		return mPitch;
	}

	float FPSCameraControl::getYaw() const
	{
		return mYaw;
	}

	void FPSCameraControl::setCursorDelta(double x, double y) 
	{
		markDirty();
		mCursorDx = x;
		mCursorDy = y;
	}

	void FPSCameraControl::setDirection(vec3 dir)
	{
		markDirty();
		vec3 normalizedDir = normalize(dir);
		setPitch	(asin(normalizedDir.y) * (normalizedDir.y < 0 ? -1. : 1.));
		setYaw		(atan2(normalizedDir.z, normalizedDir.x));
	}

	glm::vec3 FPSCameraControl::getDirection() const
	{
		float radYaw = (mYaw);
		float radPitch = (mPitch);
		vec3 ret;
		ret.x = cos(radPitch) * cos(radYaw);
		ret.y = sin(radPitch);
		ret.z = cos(radPitch) * sin(radYaw);
		return normalize(ret);
	}

	void FPSCameraControl::setSensitivity(float sensitivity)
	{
		mSensitivity = sensitivity;
	}

	void FPSCameraControl::setLocation(vec3 location)
	{
		markDirty();
		mLocation = location;
	}

	void FPSCameraControl::update(float deltaTime)
	{
		if (!mDirty) {
			return;
		}

		mYaw	+= mSensitivity * mCursorDx;
		if (mYaw > PI2) {
			mYaw -= PI2;
		}
		else if (mYaw < 0.) {
			mYaw += PI2;
		}

		mPitch	+= mSensitivity * mCursorDy;

		mPitch = clamp(mPitch, radians(- 89.5f),radians(89.5f));

		mDirty = false;
		mCursorDx = 0;
		mCursorDy = 0;
	}

	void FPSCameraControl::markDirty()
	{
		mDirty = true;
	}

}

