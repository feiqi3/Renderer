#ifndef FPS_CAMERA_CONTROL_H
#define FPS_CAMERA_CONTROL_H
#include "common/CommonMath.h"

namespace Render {
	class FPSCameraControl {
	public:
		FPSCameraControl		( );
		void	setPitch		(float rad);
		void	setYaw			(float rad);
		
		float	getPitch		( )const;
		float	getYaw			( )const;

		void	setCursorDelta	( double x,double y );

		void	setDirection	( vec3 dir);
		vec3	getDirection	( )const;
		void	setSensitivity	( float sensitivity );
		void	setLocation		( vec3 location );

		void	update			( float deltaTime );
		void	markDirty		();
	private:
		float	mSensitivity;
		float	mPitch, mYaw;
		double  mCursorDx, mCursorDy;
		vec3	mLocation;
		bool	mDirty = false;
	};
}

#endif