#pragma once

#include<queue>
#include <mutex>

template<typename T>
class ThreadSafeQueue
{
public:
	ThreadSafeQueue();
	ThreadSafeQueue(const ThreadSafeQueue& _copy);

	//将一个值推送到队列的末尾
	void Push(T _value);

	//尝试从队首将一个值出栈，如果队列为空返回fasle
	bool TryPop(T& _value);

	//检查队列是否为空
	bool Empty() const;

	//获取队列的大小
	size_t Size() const;

private:
	std::queue<T> m_Queue;
	mutable std::mutex m_Mutex;
};

template<typename T>
size_t ThreadSafeQueue<T>::Size() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Queue.size();
}

template<typename T>
bool ThreadSafeQueue<T>::Empty() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_Queue.empty();
}

template<typename T>
bool ThreadSafeQueue<T>::TryPop(T& _value)
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	if (m_Queue.empty())
	{
		return false;
	}

	_value = m_Queue.front();
	m_Queue.pop();

	return true;
}

template<typename T>
void ThreadSafeQueue<T>::Push(T _value)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Queue.push(std::move(_value));
}

template<typename T>
ThreadSafeQueue<T>::ThreadSafeQueue(const ThreadSafeQueue& _copy)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Queue = _copy.m_Queue;
}

template<typename T>
ThreadSafeQueue<T>::ThreadSafeQueue()
{
}
