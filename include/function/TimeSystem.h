#ifndef TIME_SYSTEM_H_
#define TIME_SYSTEM_H_

#include "common/Singleton.h"
#include <chrono>
#include <string>

namespace Render {

	class TimeSystem : public Singleton<TimeSystem> {
	public:
		TimeSystem();
		~TimeSystem() = default;

		void init();
		void update();

		float getDeltaTime() const { return mDeltaTime; }
		double getDeltaTime64() const { return mDeltaTime64; }

		float getTimeElapsed() const { return mTimeElapsed; }
		double getTimeElapsed64() const { return mTimeElapsed64; }

		std::chrono::system_clock::time_point getSystemClock() const;

		std::string getFormattedSystemTime(const std::string& format = "%Y-%m-%d %H:%M:%S") const;
	public:
		float	getRenderFrameTime()const;
		void	renderFrameBegin();
		void	renderFrameEnd();

		float	getLogicFrameTime()const;
		void	logicFrameBegin();
		void	logicFrameEnd();
	private:
		std::chrono::steady_clock::time_point mStartTime;
		std::chrono::steady_clock::time_point mLastFrameTime;

		float mDeltaTime = 0.0f;
		double mDeltaTime64 = 0.0;

		float mTimeElapsed = 0.0f;
		double mTimeElapsed64 = 0.0;
	
		std::chrono::steady_clock::time_point mRenderFrameStart;
		std::chrono::steady_clock::time_point mRenderFrameEnd;
		float mRenderFrameTime = 0.;

		std::chrono::steady_clock::time_point mLogicFrameStart;
		std::chrono::steady_clock::time_point mLogicFrameEnd;
		float mLogicFrameTime = 0.;
	};

} // namespace Render

#endif //!TIME_SYSTEM_H_