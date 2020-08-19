#pragma once
#include <windows.h>

/*临界区对象，线程同步*/
class CCritSec
{
public:
	CCritSec() { ::InitializeCriticalSection(&m_csCritSec); }
	~CCritSec() { ::DeleteCriticalSection(&m_csCritSec); }

	CCritSec(const CCritSec& critSec)
	{
		this->m_csCritSec = critSec.m_csCritSec;
	}
	CCritSec& operator=(const CCritSec& critSec)
	{
		if (this != &critSec)
		{
			this->m_csCritSec = critSec.m_csCritSec;
		}

		return *this;
	}

public:
	void Lock() { ::EnterCriticalSection(&m_csCritSec); }
	void Unlock() { ::LeaveCriticalSection(&m_csCritSec); }

private:
	CRITICAL_SECTION m_csCritSec;
};

/*临界区锁对象*/
class CAutoLock {
	CAutoLock(const CAutoLock& refAutoLock);
	CAutoLock& operator=(const CAutoLock& refAutoLock)
	{
		if (this != &refAutoLock)
		{
			this->m_pLock = refAutoLock.m_pLock;
		}
		return *this;
	}

protected:
	CCritSec* m_pLock;

public:
	CAutoLock(CCritSec* plock)
	{
		m_pLock = plock;
		m_pLock->Lock();
	};

	~CAutoLock() {
		m_pLock->Unlock();
	};
};
