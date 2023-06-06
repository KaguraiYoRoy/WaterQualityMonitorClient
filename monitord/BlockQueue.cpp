#include "BlockQueue.h"

template<typename T>
inline BlockQueue<T>::BlockQueue() { }


template<typename T>
inline BlockQueue<T>::~BlockQueue() { }

template<typename T>
inline void BlockQueue<T>::Put(const T& x) {
	std::unique_lock<std::mutex> guard(m_mutex);
	m_queue.push_back(x);
	m_cond.notify_all();
}

template<typename T>
inline void BlockQueue<T>::Put(T&& x)
{
	std::unique_lock<std::mutex> guard(m_mutex);
	m_queue.push_back(move(x));
	m_cond.notify_all();
}

template<typename T>
inline T BlockQueue<T>::Take()
{
	std::unique_lock<std::mutex> guard(m_mutex);
	while (m_queue.size() == 0)
		m_cond.wait(guard);
	T front(move(m_queue.front()));
	m_queue.pop_front();
	return move(front);
}


template<typename T>
inline size_t BlockQueue<T>::Size()
{
	std::unique_lock<std::mutex> guard(m_mutex);
	return m_queue.size();
}
