#include <iostream>
#include <chrono>
#include "ThreadPool.hpp"

void fun1()
{
	std::cout << "working in thread " << std::this_thread::get_id() << std::endl;
}

void fun2(int i)
{
	std::cout << "task " << i << " working in thread " << std::this_thread::get_id() << std::endl;
}

int main(int argc, char* argv[])
{
	ThreadPool threadPool(3);
	threadPool.start();
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	for(int i=0;i<6;i++)
	{
		threadPool.appendTask(std::bind(fun2, i));
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	threadPool.stop();
	getchar();
	return 0;
}
