#pragma once

#include <Windows.h>

/* ����ģ�� */
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

/*�ٽ��������߳�ͬ��*/
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

/*�ٽ���������*/
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

/*�ڴ������*/
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
		/*�������ֱ��Ϊm_pPtr��ȡ����ֵ�Ļ�,һ��reallocʧ�ܻ᷵��NULL,����m_pPtr�ڴ汾Ӧ���ǲ��ı��,��˻ᵼ��m_pPtrԭָ�����ݶ�ʧ������ڴ�������й¶*/
		/*����ʹ��һ��m_pBase��Ϊ�н鴫��m_pPtrָ��*/
		m_pBase = (PBYTE)realloc(m_pPtr, m_nSize);
		/*���m_pBase = NULL,�൱��malloc(m_nSize),m_pPtr�ڴ��ַ����;���m_nSize = 0,realloc�ɹ��൱��free(m_pPtr),��ʱӦ�ñ����ٴ�free,������һ��ָ��free���λ��������;*/
		/*���m_pPtrָ����ڴ����㹻���ڴ�,realloc�ɹ�����ԭ�ڴ��ַ;�������,realloc�ɹ��������ڴ��ַ,ԭ�ڴ��ַ��free,��ʱӦ�ñ����ٴ�free*/
		/*���ݸ�realloc��ָ������Ⱦ���malloc/calloc/realloc�ȷ�����ڴ��*/
		if (NULL != m_pBase || m_pBase != m_pPtr)
		{
			m_pPtr = m_pBase;
		}
		return 1;
	}
	UINT DeAllocateBuffer(UINT nRequestedSize = 0)
	{
		/*����Ұָ��,free֮����ΪNULL*/
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
		vector<BYTE>().swap(this->m_vecBuffer);
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