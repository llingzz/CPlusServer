#pragma once
#include <process.h>

// 自定义线程接口类
interface IThread
{
	// 状态判断
	virtual bool IsRunning() = 0;
	// 启动线程
	virtual bool StartThread() = 0;
	// 停止线程
	virtual bool StopThread(DWORD dwWaitSeconds) = 0;
	// 中止线程
	virtual bool TerminateThread(DWORD dwExitCode) = 0;
};
// 自定义线程类
class CThread : public IThread
{
	// 函数定义
public:
	CThread();
	virtual ~CThread();

	virtual bool IsRunning();
	virtual bool StartThread();
	virtual bool StopThread(DWORD dwWaitSeconds = INFINITE);
	virtual bool TerminateThread(DWORD dwExitCode);

	UINT GetThreadID() { return m_uiThreadID; }
	HANDLE GetThreadHandle() { return m_hThreadHandle; }

private:
	virtual bool Run() = 0;
	static unsigned __stdcall ThreadFunction(LPVOID lpParam);

private:
	volatile bool	m_bRun;
	UINT			m_uiThreadID;
	HANDLE			m_hThreadHandle;
};