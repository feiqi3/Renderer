#include "function/TimeSystem.h"
#include <iomanip>
#include <sstream>
#include <ctime>

namespace Render {

	TimeSystem::TimeSystem() {
		init();
	}

	void TimeSystem::init() {
		mStartTime = std::chrono::steady_clock::now();
		mLastFrameTime = mStartTime;

		mDeltaTime = 0.0f;
		mDeltaTime64 = 0.0;
		mTimeElapsed = 0.0f;
		mTimeElapsed64 = 0.0;
	}

	void TimeSystem::update() {
		auto currentClock = std::chrono::steady_clock::now();

		auto duration = currentClock - mLastFrameTime;
		mDeltaTime64 = std::chrono::duration<double>(duration).count();

		if (mDeltaTime64 <= 0.0) {
			mDeltaTime64 = 1.0 / 60.0; 
		}
		if (mDeltaTime64 > 0.1) {
			mDeltaTime64 = 0.1;
		}

		mDeltaTime = static_cast<float>(mDeltaTime64);
		mLastFrameTime = currentClock;

		auto totalDuration = currentClock - mStartTime;
		mTimeElapsed64 = std::chrono::duration<double>(totalDuration).count();
		mTimeElapsed = static_cast<float>(mTimeElapsed64);
	}

	std::chrono::system_clock::time_point TimeSystem::getSystemClock() const {
		return std::chrono::system_clock::now();
	}

	std::string TimeSystem::getFormattedSystemTime(const std::string& format) const {
		auto now = std::chrono::system_clock::now();
		auto timeT = std::chrono::system_clock::to_time_t(now);
		std::tm localTime;
#if defined(_WIN32) || defined(_WIN64)
		localtime_s(&localTime, &timeT); 
#else
		localtime_r(&timeT, &localTime); 
#endif

		std::stringstream ss;
		ss << std::put_time(&localTime, format.c_str());
		return ss.str();
	}

	float TimeSystem::getRenderFrameTime() const
	{
		return mRenderFrameTime;
	}

	void TimeSystem::renderFrameBegin()
	{
		mRenderFrameStart = std::chrono::steady_clock::now();
	}

	void TimeSystem::renderFrameEnd() {
		mRenderFrameEnd = std::chrono::steady_clock::now();
		auto  dur = mRenderFrameEnd - mRenderFrameStart;
		float timeDurMs = (std::chrono::duration_cast<std::chrono::microseconds>(dur).count()) / 1000.f;
		mRenderFrameTime = timeDurMs;
	}

	float TimeSystem::getLogicFrameTime() const
	{
		return mLogicFrameTime;
	}

	void TimeSystem::logicFrameBegin()
	{
		mLogicFrameStart = std::chrono::steady_clock::now();
	}

	void TimeSystem::logicFrameEnd()
	{
		mLogicFrameEnd = std::chrono::steady_clock::now();
		auto  dur = mRenderFrameEnd - mRenderFrameStart;
		float timeDurMs = (std::chrono::duration_cast<std::chrono::microseconds>(dur).count()) / 1000.f;
		mLogicFrameTime = timeDurMs;
	}

} // namespace Render