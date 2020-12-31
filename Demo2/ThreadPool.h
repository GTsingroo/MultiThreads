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
	/*�����߳���Ҫ���еĺ���,���ϵĴ����������ȡ����ִ��*/
	static void *Worker(void* arg);
	void run();

private:
	std::vector<std::thread> workThreads; /*�����߳�*/
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
		std::cout << "������" << i << "���߳� " << std::endl;
		/*
		std::thread temp(worker, this);
		�����ȹ����ٲ���
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
	/*������������ʱһ��Ҫ��������Ϊ���������̹߳���*/
	queueMutex.lock();
	tasksQueue.push(request);
	queueMutex.unlock();
	condition.notify_one();//�̳߳���ӽ�ȥ��������ȻҪ֪ͨ�ȴ����߳�
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
				request->process();//������ĳ�Ա����
			}
		}
	}
}


