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


	void FPSCameraControl::setPitch(float angle)
	{
		markDirty();
		mPitch = angle;
	}

	void FPSCameraControl::setYaw(float angle)
	{
		markDirty();
		mYaw = angle;
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
		float radYaw = radians(mYaw);
		float radPitch = radians(mPitch);
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

		mYaw	+= deltaTime * mSensitivity * mCursorDx;
		if (mYaw > 360.) {
			mYaw -= 360.;
		}
		else if (mYaw < 0.) {
			mYaw += 360.;
		}

		mPitch	+= deltaTime * mSensitivity * mCursorDy;

		clamp(mPitch, -89.5f, 89.5f);

		mDirty = false;
		mCursorDx = 0;
		mCursorDy = 0;
	}

	void FPSCameraControl::markDirty()
	{
		mDirty = true;
	}

}

