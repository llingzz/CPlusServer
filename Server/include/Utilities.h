#pragma once

#include <Windows.h>

/* 单例模板 */
template<typename T>
class CSingle {

protected:
	CSingle(void) {}
	~CSingle(void) {}

private:
	CSingle(const CSingle& singleton);

public:
	static T* getInstance()
	{
		if (m_pInstance)
		{
			return m_pInstance;
		}
		m_pInstance = new T;
		return m_pInstance;
	}

	static void DelInstance()
	{
		if (m_pInstance)
		{
			delete m_pInstance;
			m_pInstance = NULL;
		}
	}

private:
	static T* m_pInstance;
};

template<typename T>
T* CSingle<T>::m_pInstance = NULL;

/*临界区对象，线程同步*/
class CritSec : public CSingle<CritSec>
{
public:
	CritSec() { ::InitializeCriticalSection(&m_csCritSec); }
	~CritSec() { ::DeleteCriticalSection(&m_csCritSec); }

	CritSec(const CritSec& critSec)
	{
		this->m_csCritSec = critSec.m_csCritSec;
	}
	CritSec& operator=(const CritSec& critSec)
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
	CAutoLock(const CAutoLock &refAutoLock);
	CAutoLock &operator=(const CAutoLock &refAutoLock);

protected:
	CritSec * m_pLock;

public:
	CAutoLock(CritSec * plock)
	{
		m_pLock = plock;
		m_pLock->Lock();
	};

	~CAutoLock() {
		m_pLock->Unlock();
	};
};

/*内存管理类*/
class CBuffer
{
public:
	CBuffer()
	{
		m_nSize = 0;
		m_pBase = /*(PBYTE)malloc(sizeof(BYTE))*/NULL;
		m_pPtr = (PBYTE)malloc(sizeof(BYTE));
	}
	virtual ~CBuffer()
	{
		ClearBuffer();
	}

	void ClearBuffer()
	{
		m_nSize = 0;
		DeAllocateBuffer();
	}
	UINT GetBufferLen()
	{
		return m_nSize;
	}

	UINT Delete(UINT nSize)
	{
		return 0;
	}
	UINT Read(PBYTE pData, UINT nSize)
	{
		return 0;
	}
	UINT Write(PBYTE pData, UINT nSize)
	{
		if (NULL == pData || nSize <= 0)
		{
			cout << "the pData is NULL OR nSize == 0 !" << endl;
			return -1;
		}

		ReAllocateBuffer(nSize);
		memcpy(m_pPtr + (m_nSize - nSize), pData, nSize);

		return 1;
	}

	int Scan(PBYTE pScan, UINT nPos)
	{
		return 0;
	}
	UINT Insert(PBYTE pData, UINT nSize)
	{
		return 0;
	}

	void Copy(CBuffer& buffer)
	{
		memcpy(m_pPtr, buffer.GetBuffer(), buffer.GetBufferLen());
	}

	PBYTE GetBuffer()
	{
		return m_pPtr;
	}

	// Methods
protected:
	UINT ReAllocateBuffer(UINT nRequestedSize)
	{
		m_nSize += nRequestedSize;
		/*这里如果直接为m_pPtr获取返回值的话,一旦realloc失败会返回NULL,但是m_pPtr内存本应该是不改变的,因此会导致m_pPtr原指向内容丢失，造成内存游离与泄露*/
		/*这里使用一个m_pBase作为中介传递m_pPtr指针*/
		m_pBase = (PBYTE)realloc(m_pPtr, m_nSize);
		/*如果m_pBase = NULL,相当于malloc(m_nSize),m_pPtr内存地址不变;如果m_nSize = 0,realloc成功相当于free(m_pPtr),这时应该避免再次free,连续对一个指针free两次会出现问题;*/
		/*如果m_pPtr指向的内存有足够的内存,realloc成功返回原内存地址;如果不够,realloc成功返回新内存地址,原内存地址被free,此时应该避免再次free*/
		/*传递给realloc的指针必须先经过malloc/calloc/realloc等分配过内存的*/
		if (NULL != m_pBase || m_pBase != m_pPtr)
		{
			m_pPtr = m_pBase;
		}
		return 1;
	}
	UINT DeAllocateBuffer(UINT nRequestedSize = 0)
	{
		/*避免野指针,free之后置为NULL*/
		free(m_pPtr);
		m_pPtr = NULL;
		free(m_pBase);
		m_pBase = NULL;
		return 1;
	}
	UINT GetMemSize()
	{
		return 0;
	}

	// Attributes
protected:
	PBYTE	m_pBase;
	PBYTE	m_pPtr;
	UINT	m_nSize;
};