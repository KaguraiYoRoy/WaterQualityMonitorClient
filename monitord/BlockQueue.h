#include<mutex>
#include<list>
#include<condition_variable>

template <typename T>
class BlockQueue
{
public:
	BlockQueue(){}

	~BlockQueue(){}

	void Put(const T& x)
	{
		std::unique_lock<std::mutex> guard(m_mutex);
		m_queue.push_back(x);
		m_cond.notify_all();
	}

	void Put(T&&x)
	{
		std::unique_lock<std::mutex> guard(m_mutex);
		m_queue.push_back(std::move(x));
		m_cond.notify_all();
	}

	T Take()
	{
		std::unique_lock<std::mutex> guard(m_mutex);
		while (m_queue.size() == 0)
			m_cond.wait(guard);
		T front(std::move(m_queue.front()));
		m_queue.pop_front();
		return std::move(front);
	}

	size_t Size()
	{
		std::unique_lock<std::mutex> guard(m_mutex);
		return m_queue.size();
	}

private:
	mutable std::mutex m_mutex;
	std::condition_variable m_cond;
	std::list<T>     m_queue;
};
