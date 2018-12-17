#pragma once

#include <Windows.h>
#include <vector>

/* 单例模板 */
template<typename T>
class CSingle {

protected:
	CSingle(void) {}
	~CSingle(void) {}

private:
	CSingle(const CSingle& singleton);

public:
	static T* GetInstance()
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
class CCritSec : public CSingle<CCritSec>
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
	CAutoLock(const CAutoLock &refAutoLock);
	CAutoLock &operator=(const CAutoLock &refAutoLock);

protected:
	CCritSec * m_pLock;

public:
	CAutoLock(CCritSec * plock)
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
class CBufferEx{
public:
	CBufferEx(){}
	CBufferEx(const CBufferEx& buffer)
	{
		*this = buffer;
	}
	CBufferEx(const BYTE* pBytes, int nLen)
	{
		this->Copy(pBytes, nLen);
	}
	~CBufferEx()
	{
		this->Clear();
	}

	CBufferEx & operator = (const CBufferEx &buffer)
	{
		this->Clear();
		if (!buffer.IsEmpty())
		{
			m_vecBuffer.insert(m_vecBuffer.begin(), buffer.GetBuffer().begin(), buffer.GetBuffer().end());
		}
		return *this;
	}
	CBufferEx & operator += (const CBufferEx& buffer)
	{
		if (!buffer.IsEmpty())
		{
			m_vecBuffer.insert(m_vecBuffer.end(), buffer.GetBuffer().begin(), buffer.GetBuffer().end());
		}
	}

public:
	const BYTE* c_Bytes() const
	{
		return &m_vecBuffer[0];
	} 
	const std::vector<BYTE>& GetBuffer() const
	{
		return m_vecBuffer;
	}

	void Write(const BYTE& pBytes)
	{
		m_vecBuffer.push_back(pBytes);
	}
	void Write(const BYTE* pBytes, int nLen)
	{
		if (NULL == pBytes || 0 == nLen)
		{
			return;
		}
		m_vecBuffer.resize(this->GetLength() + nLen, 0);
		memcpy(&m_vecBuffer[0] + this->GetLength() - nLen, pBytes, nLen);
	}

	void Insert(int nStartIndex, const BYTE* pBytes, int nLen)
	{
		if (NULL == pBytes || 0 == nLen || nStartIndex < 0)
		{
			return;
		}
		int nSize = this->GetLength();
		if (nStartIndex > nSize)
		{
			return;
		}
		if (nStartIndex == nSize)
		{
			this->Write(pBytes, nLen);
		}
		else if ((nStartIndex + nLen) < nSize)
		{
			memcpy(&m_vecBuffer[0] + nStartIndex, pBytes, nLen);
		}
		else
		{
			m_vecBuffer.resize(nStartIndex + nLen);
			memcpy(&m_vecBuffer[0] + nStartIndex, pBytes, nLen);
		}
	}

	BYTE* Read(int& nLen) const
	{
		nLen = this->GetLength();
		if (this->IsEmpty())
		{
			return NULL;
		}
		BYTE* pBytes = new BYTE[nLen];
		memcpy(pBytes, &m_vecBuffer[0], nLen);
		return pBytes;
	}

	void Copy(const BYTE* pBytes, int nLen)
	{
		this->Clear();
		if (NULL == pBytes || nLen == 0)
		{
			return;
		}
		m_vecBuffer.resize(nLen, 0);
		memcpy(&m_vecBuffer[0], pBytes, nLen);
	}

	void Clear()
	{
		std::vector<BYTE>().swap(this->m_vecBuffer);
	}
	int GetLength() const
	{
		return m_vecBuffer.size();
	}
	bool IsEmpty() const
	{
		return (m_vecBuffer.size() == 0);
	}

private:
	std::vector<BYTE> m_vecBuffer;
};

//开源日志库spdlog
/*The MIT License (MIT)

Copyright (c) 2016 Gabi Melman.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
class CLog : public CSingle<CLog>
{
public:
	CLog(){}
	~CLog(){}

};