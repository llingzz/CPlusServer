#pragma once

#include <Windows.h>
#include <vector>
#include <limits.h>
#include <stddef.h>
#include <fstream>

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

//自定义日志记录类
class CLog : public CSingle<CLog>{
public:
	CLog()
	{
		m_nLogLevel = enDEFAULT;
		m_pFp = NULL;
		ZeroMemory(m_szFilePath, MAX_PATH);
		ZeroMemory(m_szFileName, MAX_PATH);
		ZeroMemory(m_szLogRrefix, BASE_DATA_BUF_SIZE);
		ZeroMemory(m_szLogContent, BASE_DATA_BUF_SIZE);
	}
	~CLog()
	{
		m_nLogLevel = enDEFAULT;
		fclose(m_pFp);
		m_pFp = NULL;
		ZeroMemory(m_szFilePath, MAX_PATH);
		ZeroMemory(m_szFileName, MAX_PATH);
		ZeroMemory(m_szLogRrefix, BASE_DATA_BUF_SIZE);
		ZeroMemory(m_szLogContent, BASE_DATA_BUF_SIZE);
	}

public:
	void WriteLogFile(const char* fmt, ...)
	{
		CAutoLock lock(&m_csLock);
		GetLocalTime(&m_sysTime);
		GetCurrentTimeRrefix();
		GetCurrentLevel(m_nLogLevel);
		GetFilePathAndName();

		va_list ap;
		va_start(ap, fmt);
		vsprintf_s(m_szLogContent, fmt, ap);
		va_end(ap);

		string szRes = m_szLogRrefix;
		szRes.append(m_szLogContent);
		strcpy_s(m_szLogContent, szRes.length() + 1, szRes.c_str());

		if (NULL == m_pFp)
		{
			fopen_s(&m_pFp, m_szFilePath, "a+");
			while (NULL == m_pFp)
			{
				fopen_s(&m_pFp, m_szFilePath, "a+");//确保文件能被打开，日志能被写入
			}
		}
		fwrite(m_szLogContent, strlen(m_szLogContent), 1, m_pFp);
		fwrite("\n", 1, 1, m_pFp);
		fflush(m_pFp);
		ftell(m_pFp);

		fclose(m_pFp);
		m_pFp = NULL;
	}
	void WriteLogConsole(const char* fmt, ...)
	{
		CAutoLock lock(&m_csLock);
		GetLocalTime(&m_sysTime);
		GetCurrentTimeRrefix();
		GetCurrentLevel(m_nLogLevel);

		va_list ap;
		va_start(ap, fmt);
		vsprintf_s(m_szLogContent, fmt, ap);
		va_end(ap);

		string szRes = m_szLogRrefix;
		szRes.append(m_szLogContent);
		strcpy_s(m_szLogContent, szRes.length() + 1, szRes.c_str());
		printf_s("%s\n", m_szLogContent);
	}
	void WriteLogFileEx(const char* fmt, ...)
	{
		CAutoLock lock(&m_csLock);
		GetLocalTime(&m_sysTime);
		GetCurrentTimeRrefix();
		GetCurrentLevel(m_nLogLevel);
		GetFilePathAndName();

		va_list ap;
		va_start(ap, fmt);
		vsprintf_s(m_szLogContent, fmt, ap);
		va_end(ap);

		string szRes = m_szLogRrefix;
		szRes.append(m_szLogContent);
		strcpy_s(m_szLogContent, szRes.length() + 1, szRes.c_str());

		ofstream log;
		log.open(m_szFilePath, ios::out | ios::app);
		while (!log.is_open())
		{
			log.open(m_szFilePath, ios::out | ios::app);
		}
		log.write(m_szLogContent, strlen(m_szLogContent));
		log.write("\n", 1);
		log.close();
	}
	CLog* SetLogLevel(int nLevel)
	{
		m_nLogLevel = nLevel;
		return this;
	}

private:
	void GetCurrentTimeRrefix()
	{
		sprintf_s(m_szLogRrefix, "[%04d-%02d-%02d %02d:%02d:%02d:%3d]", m_sysTime.wYear, m_sysTime.wMonth, m_sysTime.wDay, m_sysTime.wHour, m_sysTime.wMinute, m_sysTime.wSecond, m_sysTime.wMilliseconds);
	}
	void GetCurrentLevel(int nLevel)
	{
		char szLoglevel[8];
		switch (nLevel)
		{
		case enDEBUG:
			strcpy(szLoglevel, "DEBUG");
			break;
		case enWARN:
			strcpy(szLoglevel, "WARN");
			break;
		case enTRACE:
			strcpy(szLoglevel, "TRACE");
			break;
		case enERROR:
			strcpy(szLoglevel, "ERROR");
			break;
		case enFATAL:
			strcpy(szLoglevel, "FATAL");
			break;
		case enINFO:
		default:
			strcpy(szLoglevel, "INFO");
			break;
		}
		sprintf_s(m_szLogRrefix, "%s[%s]:", m_szLogRrefix, szLoglevel);
	}
	void GetFilePathAndName()
	{
		sprintf_s(m_szFileName, "\\%s%04d%02d%02d.log", SERVER_NAME, m_sysTime.wYear, m_sysTime.wMonth, m_sysTime.wDay);

		TCHAR szFilePath[MAX_PATH];
		GetModuleFileName(GetModuleHandle(NULL), szFilePath, sizeof(szFilePath));
		TcharToChar(szFilePath, m_szFilePath);
		const char* ptr = strrchr(m_szFilePath, '\\');

		string strStr = m_szFilePath;
		auto nIndex = strStr.find(ptr);
		string strRes = strStr.substr(0, nIndex);
		strRes.append(m_szFileName);

		strcpy_s(m_szFilePath, strRes.length() + 1, strRes.c_str());
	}

private:
	int         m_nLogLevel;
	FILE*		m_pFp;
	CCritSec	m_csLock;
	char		m_szFilePath[MAX_PATH];
	char		m_szFileName[MAX_PATH];
	char		m_szLogRrefix[BASE_DATA_BUF_SIZE];
	char		m_szLogContent[BASE_DATA_BUF_SIZE];
	SYSTEMTIME	m_sysTime;
};