#pragma once
#include<thread>
#include<condition_variable>
#include<atomic>
#include<mutex>
#include<vector>
#include <queue>

/**
 * \brief 头文件hpp实现了线程池的创建与销毁
 * 头文件hpp,没有using namespace std，不可包含全局对象和全局函数。
 */
class ThreadPool
{
public:
	using Task = std::function<void()>;
	//explicit的作用不明
	//explicit ThreadPool(int num):m_thread_num{num},m_is_running{false}{}
	ThreadPool(int num):m_thread_num{num},m_is_running{false}{}
	~ThreadPool()
	{
		if (m_is_running)
			stop();
	}
	void start()
	{
		m_is_running = true;
		for(int i=0;i<m_thread_num;++i)
		{
			m_threads.emplace_back(std::thread(&ThreadPool::run, this));
		}
	}
	void stop()
	{
		{
			// stop thread pool, should notify all threads to wake
			std::unique_lock<std::mutex> lk(m_mutex);
			m_is_running = false;
			m_cond.notify_all();// must do this to avoid thread block
		}
		// terminate every thread job
		for(std::thread& t:m_threads)
		{
			if (t.joinable())
				t.join();
		}
	
	}

	void appendTask(const Task&task)
	{
		if(m_is_running)
		{
			std::unique_lock<std::mutex>lk(m_mutex);
			m_tasks.push(task);
			m_cond.notify_one();// wake a thread to to the task
		}
	}

	/**
	 * \brief 判断任务序列是否为空
	 * \return 
	 */
	bool isEmpty()
	{
		if (!m_tasks.empty())
			return false;
		return true;
	}

private:
	void run()
	{
		printf("begin work thread:%d\n", std::this_thread::get_id());
		// every thread will compete to pick up task from the queue to do the task
		while (m_is_running)
		{
			Task task;
			{
				std::unique_lock<std::mutex>lk(m_mutex);
				if(!m_tasks.empty())
				{
					// if tasks not empty, must finish the task whether thread pool is running or not
					task = m_tasks.front();
					m_tasks.pop();
				}
				else if (m_is_running&&m_tasks.empty())
					m_cond.wait(lk);
			}
			if (task)
				task();// do the task
		}
		printf("end work thread: %d\n", std::this_thread::get_id());
	}


public:
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&other)=delete;
private:
	std::atomic_bool m_is_running;
	std::mutex m_mutex;
	std::condition_variable m_cond;
	int m_thread_num;
	std::vector<std::thread>m_threads;
	std::queue<Task>m_tasks;

};