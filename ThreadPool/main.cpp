#include <iostream>
#include <chrono>
#include "ThreadPool.hpp"
class Test
{
public:

	void fun1()
	{
		std::cout <<"No paras "<< "working in thread " << std::this_thread::get_id() << std::endl;
	}

	static void fun2()
	{
		std::cout << "No paras " << "working in thread " << std::this_thread::get_id() << std::endl;
	}
};


void fun(int i)
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
		threadPool.appendTask(std::bind(fun, i));
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
	
	Test tt_bind;
	threadPool.appendTask(std::bind(&Test::fun1, &tt_bind));
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	threadPool.appendTask(std::bind(&Test::fun2));
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	
	threadPool.stop();
	getchar();
	return 0;
}
