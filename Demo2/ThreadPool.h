#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <iostream>
#include <condition_variable>

//#include <memory> //unique_ptr
const int MAX_THREADS = 100;

template<typename T>
class ThreadPool
{
public:
	ThreadPool(int number = 1);
	~ThreadPool();
	bool Append(T* request);

private:
	/*工作线程需要运行的函数,不断的从任务队列中取出并执行*/
	static void *Worker(void* arg);
	void run();

private:
	std::vector<std::thread> workThreads; /*工作线程*/
	std::queue<T*> tasksQueue;
	std::mutex queueMutex;
	std::condition_variable condition;
	bool stop;


};

template <typename T>
ThreadPool<T>::ThreadPool(int number):stop(false)
{
	if (number<0 || number>MAX_THREADS)
		throw std::exception();
	for(int i=0;i<number;i++)
	{
		std::cout << "创建第" << i << "个线程 " << std::endl;
		/*
		std::thread temp(worker, this);
		不能先构造再插入
		 */
		workThreads.emplace_back(Worker, this);
	}
}

template <typename T>
ThreadPool<T>::~ThreadPool()
{
	{
		std::unique_lock<std::mutex> lock(queueMutex);
		stop = true;
	}
	condition.notify_all();
	for (auto &ww : workThreads)
		ww.join();
}

template <typename T>
bool ThreadPool<T>::Append(T* request)
{
	/*操作工作队列时一定要加锁，因为他被所有线程共享*/
	queueMutex.lock();
	tasksQueue.push(request);
	queueMutex.unlock();
	condition.notify_one();//线程池添加进去了任务，自然要通知等待的线程
	return true;
}

template <typename T>
void* ThreadPool<T>::Worker(void* arg)
{
	ThreadPool* pool = (ThreadPool*)arg;
	pool->run();
	return pool;
}

template <typename T>
void ThreadPool<T>::run()
{
	while (!stop)
	{
		std::unique_lock<std::mutex>lk(queueMutex);
		condition.wait(lk, [this] {return !tasksQueue.empty(); });
		if (tasksQueue.empty())
		{
			continue;
		}
		else
		{
			T* request = tasksQueue.front();
			tasksQueue.pop();
			if(request)
			{
				request->process();//测试类的成员函数
			}
		}
	}
}


