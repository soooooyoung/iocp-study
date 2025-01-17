#pragma once
#include <chrono>
#include <functional>

class  Timer
{
public:
	Timer(std::function<void()> callback, int interval) : mStartTime(std::chrono::steady_clock::now()),
		mCallback(callback),
		mIntervalMS(interval)
	{
	}

	void Reset()
	{
		mStartTime = std::chrono::steady_clock::now();
	}

	int GetElapsedTimeMS() const
	{
		auto endTime = std::chrono::steady_clock::now();
		auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - mStartTime);
		return static_cast<int>(elapsedTime.count());
	}

	bool Update()
	{
		if (GetElapsedTimeMS() >= mIntervalMS)
		{
			if (mCallback)
			{
				mCallback();
				Reset();
				return true;
			}
		}

		return false;
	}

private:
	std::chrono::steady_clock::time_point mStartTime;
	int mIntervalMS = 0;
	std::function<void()> mCallback;
};