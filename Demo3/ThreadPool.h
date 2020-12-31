#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <future>
#include <functional>

#define MAX_THREAD_NUM 256

//线程池,可以提交变参函数或拉姆达表达式的匿名函数执行,可以获取执行返回值
//不支持类成员函数, 支持类静态成员函数或全局函数,Opteron()函数等
class ThreadPool
{
	using Task = std::function<void()>;
	// 线程池
	std::vector<std::thread> pool;
	// 任务队列
	std::queue<Task> tasks;
	// 同步
	std::mutex m_lock;
	// 条件阻塞
	std::condition_variable cv_task;
	// 是否关闭提交
	std::atomic<bool> stoped;
	//空闲线程数量
	std::atomic<int>  idlThrNum;
public:
	inline ThreadPool(unsigned short size = 4) :stoped{ false }
	{
		idlThrNum = size < 1 ? 1 : size;
		for (size = 0; size < idlThrNum; ++size)
		{   //初始化线程数量
			pool.emplace_back(
				[this]
			{ // lambda匿名函数对象
			//此处的循环体不会阻塞整体，因为该处是单个线程的lambda函数，运行在单独的线程中
				while (!this->stoped)
				{
					std::function<void()> task;
					{   // 获取一个待执行的 task
						std::unique_lock<std::mutex> lock{ this->m_lock };// unique_lock 相比 lock_guard 的好处是：可以随时 unlock() 和 lock()
						this->cv_task.wait(lock,
							[this] {
							return this->stoped.load() || !this->tasks.empty();
						}//this 是引用域外的变量 this指针
						); // wait 直到有 task
						if (this->stoped && this->tasks.empty())
							return;
						task = std::move(this->tasks.front()); // 取一个 task
						this->tasks.pop();
					}
					//--和++操作并不会影响成功启动的线程总数量，
					//因为构造的时候，所有的子线程全在上面wait处等待，并不会执行到此处来
					idlThrNum--;
					task();
					idlThrNum++;
				}
			}
			);
		}
	}
	inline ~ThreadPool()
	{
		stoped.store(true);
		cv_task.notify_all(); // 唤醒所有线程执行
		for (std::thread& thread : pool) {
			//thread.detach(); // 让线程“自生自灭”
			if (thread.joinable())
				thread.join(); // 等待任务结束， 前提：线程一定会执行完
		}
	}

public:
	/* 提交一个任务
	 调用.get()获取返回值会等待任务执行完,获取返回值
	
	commit 方法可以带任意多的参数，第一个参数是 f，后面依次是函数 f 的参数！
	(注意:参数要传struct/class的话,建议用pointer,小心变量的作用域) 可变参数模板是 c++11 的一大亮点
	 commit 直接使用只能调用stdcall函数，但有两种方法可以实现调用类成员，
	 一种是使用   bind： .commit(std::bind(&Dog::sayHello, &dog));
	 一种是用 mem_fn： .commit(std::mem_fn(&Dog::sayHello), &dog)

	delctype(expr) 用来推断 expr 的类型，和 auto 是类似的，相当于类型占位符，占据一个类型的位置；
	auto f(A a, B b) -> decltype(a+b) 是一种用法，不能写作 decltype(a+b) f(A a, B b)！ c++ 就是这么规定的！
	
	*/
	template<class F, class... Args>
	auto commit(F&& f, Args&&... args) ->std::future<decltype(f(args...))>
	{
		if (stoped.load())    // stop == true ??
			throw std::runtime_error("commit on ThreadPool is stopped.");

		using RetType = decltype(f(args...)); // typename std::result_of<F(Args...)>::type, 函数 f 的返回值类型
		//forward() 函数，类似于 move() 函数，后者是将参数右值化，
		//前者是不改变最初传入的类型的引用类型(左值还是左值，右值还是右值)；
		auto task = std::make_shared<std::packaged_task<RetType()> >(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
			);    // wtf !
		std::future<RetType> future = task->get_future();
		{    // 添加任务到队列
			std::lock_guard<std::mutex> lock{ m_lock };//对当前块的语句加锁  lock_guard 是 mutex 的 stack 封装类，构造的时候 lock()，析构的时候 unlock()
			tasks.emplace([task](){ (*task)();});// emplace相当于容器的push操作，不会发生临时拷贝，更加高级
		}
		cv_task.notify_one(); // 唤醒一个线程执行
		return future;
	}

	//空闲线程数量
	int idlCount() { return idlThrNum; }

};




