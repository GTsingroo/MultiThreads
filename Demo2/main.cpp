#include "threadPool.h"
#include<string>
using namespace std;
class Task
{
public:
	void process()
	{
		cout << "run........." << endl;
	}
};
int main(void)
{
	ThreadPool<Task> pool(6);
	std::string str;
	for(int i=0;i<15;++i)
	{
		Task *tt = new Task();
		//ʹ������ָ��
		pool.Append(tt);
		delete tt;
	}
}