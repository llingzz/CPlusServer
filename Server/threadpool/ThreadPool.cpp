#include "stdafx.h"

CThread::CThread()
{
	m_bRun = false;
	m_uiThreadID = 0;
	m_hThreadHandle = NULL;
}
CThread::~CThread()
{
	StopThread();
}
bool CThread::IsRunning()
{
	if (m_hThreadHandle != NULL)
	{
		DWORD dwRetCode = ::WaitForSingleObject(m_hThreadHandle, 0);
		if (dwRetCode == WAIT_TIMEOUT) return true;
	}
	return false;
}
bool CThread::StartThread()
{
	if (IsRunning())
	{
		return false;
	}

	if (NULL != m_hThreadHandle)
	{
		::CloseHandle(m_hThreadHandle);
		m_hThreadHandle = NULL;
		m_uiThreadID = 0;
	}

	ThreadParam stuThreadParam;
	stuThreadParam.bSuccess = true;
	stuThreadParam.hEventHandle = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	if (NULL == stuThreadParam.hEventHandle)
	{
		return false;
	}
	stuThreadParam.pThread = this;

	m_bRun = true;
	m_hThreadHandle = (HANDLE)::_beginthreadex(NULL, 0, ThreadFunction, &stuThreadParam, 0, &m_uiThreadID);

	::WaitForSingleObject(stuThreadParam.hEventHandle, INFINITE);
	::CloseHandle(stuThreadParam.hEventHandle);

	if (false == stuThreadParam.bSuccess)
	{
		StopThread();
		return false;
	}

	return true;
}
bool CThread::StopThread(DWORD dwWaitSeconds /* = INFINITE */)
{
	if (IsRunning())
	{
		m_bRun = false;
		DWORD dwRetCode = WaitForSingleObject(m_hThreadHandle, dwWaitSeconds);
		if (dwRetCode == WAIT_TIMEOUT) return false;
	}

	if (NULL != m_hThreadHandle)
	{
		CloseHandle(m_hThreadHandle);
		m_hThreadHandle = NULL;
		m_uiThreadID = 0;
	}

	return true;
}
bool CThread::TerminateThread(DWORD dwExitCode)
{
	if (IsRunning() == true)
	{
		::TerminateThread(m_hThreadHandle, dwExitCode);
	}

	if (m_hThreadHandle != NULL)
	{
		CloseHandle(m_hThreadHandle);
		m_hThreadHandle = NULL;
		m_uiThreadID = 0;
	}

	return true;
}
unsigned __stdcall CThread::ThreadFunction(LPVOID lpParam)
{
	if (NULL == lpParam)
	{
		return 0;
	}

	ThreadParam* pThreadParam = (ThreadParam*)lpParam;
	CThread* pThread = pThreadParam->pThread;

	::SetEvent(pThreadParam->hEventHandle);
	if (pThreadParam->bSuccess)
	{
		while (pThread->m_bRun)
		{
			try
			{
				if (!pThread->Run())
				{
					break;
				}
			}
			catch (...)
			{
				myLogConsoleE("线程执行异常...");
			}
		}
	}

	::_endthreadex(0);
	return 0;
}