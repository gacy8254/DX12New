#pragma once

#include <chrono>

class HighResolutionClock
{
public:
	HighResolutionClock();

	void Tick();

	//重置计时器
	void Reset();

	double GetDeltaNanoseconds() const  { return m_DeltaTime.count() * 1.0; }
	double GetDeltaMicroseconds() const { return m_DeltaTime.count() * 1e-3; }
	double GetDeltaMilliseconds() const { return m_DeltaTime.count() * 1e-6; }
	double GetDeltaSeconds() const		{ return m_DeltaTime.count() * 1e-9; }

	double GetTotalNanoseconds() const { return m_TotalTime.count() * 1.0; }
	double GetTotalMicroseconds() const { return m_TotalTime.count() * 1e-3; }
	double GetTotalMilliSeconds() const { return m_TotalTime.count() * 1e-6; }
	double GetTotalSeconds() const { return m_TotalTime.count() * 1e-9; }


private:
	//初始的时间点
	std::chrono::high_resolution_clock::time_point m_T0;
	//时间间隔
	std::chrono::high_resolution_clock::duration m_DeltaTime;
	//总时间
	std::chrono::high_resolution_clock::duration m_TotalTime;
};

