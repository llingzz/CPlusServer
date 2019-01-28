#pragma once

//----------------------------------------------------------------
//类  ：CEvent
//用途：实现事件对象的封装，主要用于线程同步
//默认构造函数生成一个手动重置的事件对象。
//线程需要等待时，调用Wait()；通知线程执行时，调用Notify()方法；
//----------------------------------------------------------------
class CEvent
{
public:
	explicit CEvent();			//构造函数，explicit关键字修饰，抑制内置类型隐式转换
	virtual ~CEvent();			//析构函数

	void Wait();				//线程等待(一直等待)
	int  Wait(int nTimeOut);	//线程等待(nTimeOut时间内等待)
	void Reset();				//重置事件信号
	void Notify();				//触发事件信号，通知线程执行

private:
	HANDLE	m_hEvent;			//事件对象句柄
};

//CTask的前置声明
class CTask;

//----------------------------------------------------------------
//结构体  ：priority_queue_cmp
//用途    ：排序任务队列的排序参数
//作为priority_queue的第三个参数，实现自定义排序任务队列
//----------------------------------------------------------------
template <class T>
struct priority_queue_cmp
{
	bool operator () (const T t1, const T t2) const
	{
		if (t1->GetPriority() > t2->GetPriority())
		{
			return true;
		}

		return false;
	}
};

//根据任务优先级进行排序的任务队列
typedef std::priority_queue<CTask*, std::vector<CTask*>, priority_queue_cmp<CTask*>> TaskQueue;
//----------------------------------------------------------------
//类  ：CTask
//用途：任务基类
//工作线程中调用的任务为该类的子类，需要重载基类的Run()方法。
//在线程内进行销毁。
//----------------------------------------------------------------
interface CTask
{
public:
	explicit CTask(int nRriority = 0);	//构造函数，explicit关键词修饰，抑制内置类型隐式转换
	virtual ~CTask();					//析构函数

	virtual void Run() = 0;				//纯虚函数接口，用于子类继承实现
	virtual void TimeOut();				//超时

	void SetPriority(int nPriority);	//设置任务优先级
	int GetPriority();					//得到任务优先级
	void SetThreadID(int nThreadID);	//设置当前任务线程ID
	int GetThreadID();					//得到当前任务线程ID

private:
	int	m_nThreadID;					//执行当前任务线程ID
	int m_nPriority;					//任务优先级，优先级越大越先执行
};

//----------------------------------------------------------------
//类  ：CTaskQueue
//用途：任务队列
//根据任务优先级排序的自定义队列类。
//----------------------------------------------------------------
class CTaskQueue
{
public:
	explicit CTaskQueue();				//构造函数，explicit关键词修饰，抑制内置类型隐式转换
	virtual ~CTaskQueue();				//析构函数

	CTask* PopTask();					//从队列弹出任务
	void PushTask(CTask* pTask);		//向队列压入任务

	void WaitTask();					//等待新任务到来
	void NotifyTask();					//通知新任务到来

	bool Empty();						//任务队列是否为空
	int Size();							//任务队列的任务数

private:
	TaskQueue m_tqTaskQueue;			//任务队列
	CCritSec m_csTaskQueue;				//临界区对象
	CEvent	m_eEvent;					//事件对象
};

//----------------------------------------------------------------
//枚举  ：enPriority
//用途  ：任务优先级枚举
//----------------------------------------------------------------
typedef enum enPriority
{
	enLowPriority,		//低优先级
	enMediumPriority,	//中优先级
	enHighPriority,		//高优先级
};

//----------------------------------------------------------------
//枚举  ：enThreadState
//用途  ：线程运行状态枚举
//----------------------------------------------------------------
typedef enum enThreadState
{
	enInitTS,			//初始状态
	enSuspendTS,		//挂起状态
	enRunningTS,		//运行状态
	enBlockedTS,		//阻塞状态
	enFinishTS,			//结束状态
};

//CThread的前置声明
class CThread;

//----------------------------------------------------------------
//类  ：CRunnable
//用途：线程执行单元
//每个CThread通过调用CRunnable的纯虚接口Run()执行具体任务逻辑。
//----------------------------------------------------------------
class CRunnable
{
public:
	explicit CRunnable(CThread* pThread = NULL);			//构造函数，explicit关键字修饰，抑制内置类型隐式转换
	virtual ~CRunnable();									//析构函数

	virtual void Run() = 0;									//纯虚函数接口，用于子类继承实现

	void SetThread(CThread* pThread);						//设置当前线程
	CThread* GetThread();									//得到当前线程

	int Wait(int nTimeOut);									//等待事件触发
	void Notify();											//通知事件触发

private:
	CThread* m_pThread;										//当前线程
	CEvent m_eEvent;										//事件对象
};

//----------------------------------------------------------------
//类  ：CThread
//用途：线程类
// C++ 线程类的基本封装。
//----------------------------------------------------------------
class CThread
{
public:
	explicit CThread(CRunnable* pRunnable);					//构造函数，explicit关键字修饰，抑制内置类型隐式转换
	virtual ~CThread();										//析构函数

	void Start(bool bSuspend = false);						//开始线程(默认当前线程未挂起)
	void Excute();											//执行线程
	void Join();											//等待线程执行完成
	void Suspend();											//挂起线程
	void Resume();											//恢复线程
	void Stop();											//停止线程

	void Wait();											//限时等待
	void Notify();											//通知线程

	void SetThreadState(enThreadState enThreadSte);			//设置线程状态
	enThreadState GetThreadState();							//得到线程状态
	void SetPriority(enPriority enThreadPriority);			//设置线程优先级
	enPriority GetPriority();								//得到线程优先级
	void SetThreadID(unsigned int uiThreadID);				//设置线程ID
	unsigned int GetThreadID();								//得到线程ID

private:
	unsigned int m_uiThreadID;								//线程ID
	HANDLE m_hThreadHandle;									//线程句柄
	CRunnable* m_pRunnable;									//CRunnable对象指针
	CEvent m_eEvent;										//事件对象
	enThreadState m_enThreadState;							//线程状态
};

static unsigned int __stdcall ThreadFunc(void* lpParam);//线程函数

//CThreadPool的前置声明
class CThreadPool;

//----------------------------------------------------------------
//类  ：CWorkerThread
//用途：工作线程
//线程池中正在负责工作的线程。
//----------------------------------------------------------------
class CWorkerThread : public CRunnable
{
public:
	explicit CWorkerThread(CThreadPool* pThreadPool = NULL);	//构造函数，explicit关键字修饰，抑制内置类型隐式转换
	virtual ~CWorkerThread();									//析构函数

	void SetTask(CTask* pTask, void* pData);					//为当前工作线程设置Task
	CThread* GetThread();										//得到当前线程

	void Run();													//线程执行
	void Suspend();												//线程挂起
	void Join();												//等待线程执行完成
	void Resume();												//恢复线程
	void Stop();												//停止线程

private:
	CThreadPool* m_pThreadPool;									//所属线程池
	CThread* m_pThread;											//对应线程
	CTask* m_pTask;												//对应任务Task
	CEvent m_eEvent;											//事件对象

	bool m_bIsExit;												//线程是否退出
};

//----------------------------------------------------------------
//类  ：CAllocThread
//用途：分配线程
//负责处理任务队列中的线程，将任务分配给空闲线程。
//----------------------------------------------------------------
class CAllocThread : public CRunnable
{
public:
	explicit CAllocThread(CThreadPool* pThreadPool = NULL);		//构造函数，explicit关键字修饰，抑制内置类型隐式转换
	virtual ~CAllocThread();									//析构函数

	void Run();													//线程执行
	void Stop();												//线程结束

private:
	CThreadPool* m_pThreadPool;									//所属线程池
	CThread* m_pThread;											//对应线程

	bool m_bIsExit;												//线程离开标志
};

//----------------------------------------------------------------
//类  ：CCleanThread
//用途：清理线程
//负责清理线程池中多余的空闲线程。
//----------------------------------------------------------------
class CCleanThread : public CRunnable
{
public:
	explicit CCleanThread(CThreadPool* pThreadPool = NULL);		//构造函数，explicit关键字修饰，抑制内置类型隐式转换
	virtual ~CCleanThread();									//析构函数

	void Run();													//线程执行
	void Stop();												//线程结束

private:
	CThreadPool* m_pThreadPool;									//所属线程池
	CThread* m_pThread;											//对应线程

	bool m_bIsExit;												//线程离开标志
};

//----------------------------------------------------------------
//类  ：CWorkerThreadList
//用途：工作线程列表
//对线程池中工作线程列表的封装。
//----------------------------------------------------------------
class CWorkerThreadList
{
public:
	explicit CWorkerThreadList();								//构造函数，explicit关键字修饰，抑制内置类型隐式转换
	virtual ~CWorkerThreadList();								//析构函数

	void PushWorkerThread(CWorkerThread* pWorkerThread);		//将工作线程压入列表
	CWorkerThread* PopWorkerThread();							//将工作线程弹出列表
	void EraseWorkerThread(CWorkerThread* pWorkerThread);		//删除列表中的工作线程
	void Clear();												//清理工作线程列表

	void Wait();												//等待
	int Wait(int nSecs);										//等待一段时间
	void Notify();												//通知

private:
	std::vector<CWorkerThread*> m_vtWorkerThreadList;			//工作线程列表
	CEvent m_eEvent;											//事件对象
};

//----------------------------------------------------------------
//类  ：CThreadPool
//用途：线程池。
//----------------------------------------------------------------
class CThreadPool
{
public:
	explicit CThreadPool(int nMaxThreads = 20,
		int nMaxIdleThreads = 10, int nMinIdleThreads = 5);		//构造函数，explicit关键字修饰，抑制内置类型隐式转换
	virtual ~CThreadPool();										//析构函数

	void Start();												//开启线程池
	void Stop();												//关闭线程池

	void CreateWorker(int nWorkerNums);							//创建工作线程
	void CleanWorkers();										//清理多余空闲线程
	void MoveToIdleList(CWorkerThread* pWorkerThread);			//将工作完成的线程移回到空闲线程列表

	void PushTask(CTask* pTask, void* pData);					//将任务压入任务队列
	CTask* PopTask();											//将任务从队列中弹出

	void HandleTask(CTask* pTask, void* pData);					//处理任务

	void WaitForTask();											//线程池等待任务
	void Notify();												//线程池接收到任务

private:
	int m_nMaxThreads;											//线程池规定最多线程数
	int m_nMinIdleThreads;										//最少空闲线程数
	int m_nMaxIdleThreads;										//最多空闲线程数
	int m_nBusyThreads;											//当前忙碌线程数
	int m_nCurrentThreads;										//当前总线程数

	bool m_bIsExit;												//线程池退出标志
	CCritSec m_csMutex;											//临界区对象

	CWorkerThreadList m_ltWorkersList;							//工作线程表
	CWorkerThreadList m_ltIdleList;								//空闲线程表

	CTaskQueue m_tqTaskQueue;									//优先任务队列

	CAllocThread* m_pAllcoThread;								//分配任务线程
	CCleanThread* m_pCleanThread;								//清理线程
};