#pragma once

#include <chrono>

class HighResolutionClock
{
public:
	HighResolutionClock();

	void Tick();

	//���ü�ʱ��
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
	//��ʼ��ʱ���
	std::chrono::high_resolution_clock::time_point m_T0;
	//ʱ����
	std::chrono::high_resolution_clock::duration m_DeltaTime;
	//��ʱ��
	std::chrono::high_resolution_clock::duration m_TotalTime;
};

