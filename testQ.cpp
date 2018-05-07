#include <thread> 
#include <chrono> 
#include <iostream>
#include <chrono> 
#include <condition_variable>
#include <mutex>
#include <deque>

std::mutex logMtx;

template<class T>
class BlockQueue
{
public:
	typedef std::unique_lock<std::mutex> TLock;
	//maxCapacity为-1，代表队列无最大限制  
	explicit BlockQueue(const int maxCapacity = -1) :m_maxCapacity(maxCapacity)
	{
	}
	size_t size()
	{
		TLock lock(m_mutex);
		return m_list.size();
	}

	void push_back(const T &item)
	{
		TLock lock(m_mutex);
		m_list.push_back(item);
		m_notEmpty.notify_all();
	}

	T pop()
	{
		TLock lock(m_mutex);
		if (!m_list.empty())
		{
			T temp = *m_list.begin();
			m_list.pop_front();
			return temp;
		}
		m_notEmpty.wait(lock);
		if (!m_list.empty())
		{
			T temp = *m_list.begin();
			m_list.pop_front();
			return temp;
		}
		
		return NULL;
	}

	bool empty()
	{
		TLock lock(m_mutex);
		return m_list.empty();
	}

	bool full()
	{
		if (false == hasCapacity)
		{
			return false;
		}

		TLock lock(m_mutex);
		return m_list.size() >= m_maxCapacity;
	}

	void stop(){
		m_notEmpty.notify_all();
	}
private:
	bool hasCapacity() const
	{
		return m_maxCapacity > 0;
	}

	typedef std::deque<T> TList;
	TList m_list;

	const int m_maxCapacity;

	std::mutex m_mutex;
	std::condition_variable m_notEmpty;
	std::condition_variable m_notFull;
};

class LogMsgQueue
{
private:
	BlockQueue<const char *> m_qLog;
	volatile bool m_isContinue;
	void LogThread();

public:
	LogMsgQueue(){}
	void Start();
	void Stop();
	void Push(const char * gameLog);
	char *Wait_Pop();
	size_t Size(){
		return m_qLog.size();
	}
};



void LogMsgQueue::LogThread(){
	while (true)
	{
		char * log;
		log = this->Wait_Pop();
		size_t currSize = this->Size();

		if (!this->m_isContinue)
		{
			logMtx.lock();
			std::cout << "--- handle log: " << log << ", curr size: " << m_qLog.size() << ", thread over";
			logMtx.unlock();
			break;
		}
		else {
			logMtx.lock();
			std::cout << "--- handle log: " << log << ", curr size: " << m_qLog.size() << std::endl;
			logMtx.unlock();
		}

		

		std::this_thread::sleep_for(std::chrono::seconds(2));
	}

	return;
}

void LogMsgQueue::Start()
{
	m_isContinue = true;
	std::thread tLog(&LogMsgQueue::LogThread, this);
	tLog.detach();
}

void LogMsgQueue::Stop()
{
	m_isContinue = false;
	this->Push(" over");
}

void LogMsgQueue::Push(const char *gameLog)
{
	if (gameLog == NULL)
	{
		return;
	}
	m_qLog.push_back(gameLog);
	logMtx.lock();
	std::cout << "+++   push log: " << gameLog << ", curr size: " << m_qLog.size() << std::endl;
	logMtx.unlock();
}

char *LogMsgQueue::Wait_Pop()
{
	return (char*)m_qLog.pop();
}

LogMsgQueue msgQ;

int main() {
	msgQ.Start();

	char logbuf[5][32] = {
		"log 1",
		"log 2",
		"log 3",
		"log 4",
		"log 5"
	};
	for (int i = 0; i < 5; i++)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
		
		/*logMtx.lock();
		std::cout << "push log: " << logbuf[i] << std::endl;
		logMtx.unlock();*/
		
		msgQ.Push(logbuf[i]);
	}
	msgQ.Stop();

	getchar();

	std::cout << "final size: " << msgQ.Size() << std::endl;
	
	getchar();
}