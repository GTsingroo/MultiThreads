#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <future>
#include <functional>

#define MAX_THREAD_NUM 256

//�̳߳�,�����ύ��κ�������ķ����ʽ����������ִ��,���Ի�ȡִ�з���ֵ
//��֧�����Ա����, ֧���ྲ̬��Ա������ȫ�ֺ���,Opteron()������
class ThreadPool
{
	using Task = std::function<void()>;
	// �̳߳�
	std::vector<std::thread> pool;
	// �������
	std::queue<Task> tasks;
	// ͬ��
	std::mutex m_lock;
	// ��������
	std::condition_variable cv_task;
	// �Ƿ�ر��ύ
	std::atomic<bool> stoped;
	//�����߳�����
	std::atomic<int>  idlThrNum;
public:
	inline ThreadPool(unsigned short size = 4) :stoped{ false }
	{
		idlThrNum = size < 1 ? 1 : size;
		for (size = 0; size < idlThrNum; ++size)
		{   //��ʼ���߳�����
			pool.emplace_back(
				[this]
			{ // lambda������������
			//�˴���ѭ���岻���������壬��Ϊ�ô��ǵ����̵߳�lambda�����������ڵ������߳���
				while (!this->stoped)
				{
					std::function<void()> task;
					{   // ��ȡһ����ִ�е� task
						std::unique_lock<std::mutex> lock{ this->m_lock };// unique_lock ��� lock_guard �ĺô��ǣ�������ʱ unlock() �� lock()
						this->cv_task.wait(lock,
							[this] {
							return this->stoped.load() || !this->tasks.empty();
						}//this ����������ı��� thisָ��
						); // wait ֱ���� task
						if (this->stoped && this->tasks.empty())
							return;
						task = std::move(this->tasks.front()); // ȡһ�� task
						this->tasks.pop();
					}
					//--��++����������Ӱ��ɹ��������߳���������
					//��Ϊ�����ʱ�����е����߳�ȫ������wait���ȴ���������ִ�е��˴���
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
		cv_task.notify_all(); // ���������߳�ִ��
		for (std::thread& thread : pool) {
			//thread.detach(); // ���̡߳���������
			if (thread.joinable())
				thread.join(); // �ȴ���������� ǰ�᣺�߳�һ����ִ����
		}
	}

public:
	/* �ύһ������
	 ����.get()��ȡ����ֵ��ȴ�����ִ����,��ȡ����ֵ
	
	commit �������Դ������Ĳ�������һ�������� f�����������Ǻ��� f �Ĳ�����
	(ע��:����Ҫ��struct/class�Ļ�,������pointer,С�ı�����������) �ɱ����ģ���� c++11 ��һ������
	 commit ֱ��ʹ��ֻ�ܵ���stdcall�������������ַ�������ʵ�ֵ������Ա��
	 һ����ʹ��   bind�� .commit(std::bind(&Dog::sayHello, &dog));
	 һ������ mem_fn�� .commit(std::mem_fn(&Dog::sayHello), &dog)

	delctype(expr) �����ƶ� expr �����ͣ��� auto �����Ƶģ��൱������ռλ����ռ��һ�����͵�λ�ã�
	auto f(A a, B b) -> decltype(a+b) ��һ���÷�������д�� decltype(a+b) f(A a, B b)�� c++ ������ô�涨�ģ�
	
	*/
	template<class F, class... Args>
	auto commit(F&& f, Args&&... args) ->std::future<decltype(f(args...))>
	{
		if (stoped.load())    // stop == true ??
			throw std::runtime_error("commit on ThreadPool is stopped.");

		using RetType = decltype(f(args...)); // typename std::result_of<F(Args...)>::type, ���� f �ķ���ֵ����
		//forward() ������������ move() �����������ǽ�������ֵ����
		//ǰ���ǲ��ı������������͵���������(��ֵ������ֵ����ֵ������ֵ)��
		auto task = std::make_shared<std::packaged_task<RetType()> >(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
			);    // wtf !
		std::future<RetType> future = task->get_future();
		{    // ������񵽶���
			std::lock_guard<std::mutex> lock{ m_lock };//�Ե�ǰ���������  lock_guard �� mutex �� stack ��װ�࣬�����ʱ�� lock()��������ʱ�� unlock()
			tasks.emplace([task](){ (*task)();});// emplace�൱��������push���������ᷢ����ʱ���������Ӹ߼�
		}
		cv_task.notify_one(); // ����һ���߳�ִ��
		return future;
	}

	//�����߳�����
	int idlCount() { return idlThrNum; }

};




