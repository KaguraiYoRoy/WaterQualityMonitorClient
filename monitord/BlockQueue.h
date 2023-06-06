#include<mutex>
#include<list>
#include<condition_variable>

template <typename T>
class BlockQueue
{
private:
	mutable std::mutex m_mutex;
	std::condition_variable m_cond;
	std::list<T>     m_queue;
public:
	BlockQueue();
	~BlockQueue();
	void Put(const T& x);
	void Put(T&& x);
	T Take();
	size_t Size();

};

