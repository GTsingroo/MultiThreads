#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include <mutex>
#include<condition_variable>
#include <cassert>

class ThreadPool
{
public:
	
	enum  TaskPriority
	{
		LEVEL0 = 0,
		LEVEL1,
		LEVEL2,
	};
	typedef std::function<void()> Task;
	typedef std::pair<TaskPriority, Task> TaskPair;

	ThreadPool(int num) :initThreadsSize{ num }, m_isStarted{ false } {}
	~ThreadPool() 
	{
		if (m_isStarted)
		{
			stop();
		}
	}

	void start()
	{
		assert(m_threads.empty());
		m_isStarted = true;
		m_threads.reserve(initThreadsSize);
		for (int i = 0; i < initThreadsSize; ++i)
			m_threads.push_back(new std::thread(std::bind(&ThreadPool::threadLoop, this)));
	}
	void stop()
	{
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_isStarted = false;
			m_cond.notify_all();

		}
		for(Threads::iterator it=m_threads.begin();it!=m_threads.end();++it)
		{
			if ((*it)->joinable())
				(*it)->join();
		}
		m_threads.clear();
	}
	void addTask(const Task& task)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		TaskPair taskPair(LEVEL2, task);
		m_tasks.push(taskPair);
		m_cond.notify_one();
	}
	void addTask(const TaskPair& taskPair)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_tasks.push(taskPair);
		m_cond.notify_one();
	}

private:
	ThreadPool(const ThreadPool&) = delete;
	const ThreadPool& operator=(const ThreadPool&) = delete;

	struct TaskPriorityCmp
	{
		bool operator()(const ThreadPool::TaskPair p1, const ThreadPool::TaskPair p2)
		{
			return p1.first > p2.first; //first的小值优先
		}
	};


	void threadLoop()
	{
		while (m_isStarted)
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			while (m_tasks.empty() && m_isStarted)
				m_cond.wait(lock);
			Task task;
			Tasks::size_type size = m_tasks.size();
			if (!m_tasks.empty() && m_isStarted)
			{
				task = m_tasks.top().second;
				m_tasks.pop();
				assert(size - 1 == m_tasks.size());
			}
			if (task)
				task();// do the task
		}
	}

	typedef std::vector<std::thread*> Threads;
	typedef std::priority_queue<TaskPair, std::vector<TaskPair>,TaskPriorityCmp> Tasks;

	Threads m_threads;
	Tasks m_tasks;
    int initThreadsSize = 3;
	std::mutex m_mutex;
	std::condition_variable m_cond;
	bool m_isStarted;
};



