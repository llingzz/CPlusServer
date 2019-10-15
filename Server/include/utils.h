#pragma once
#include <Windows.h>
#include <vector>
#include <limits.h>
#include <stddef.h>
#include <fstream>
#include <assert.h>

/*单例模板*/
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
		static T instance;
		return &instance;
	}
};

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
	CAutoLock(const CAutoLock &refAutoLock);
	CAutoLock &operator=(const CAutoLock &refAutoLock)
	{
		if (this != &refAutoLock)
		{
			this->m_pLock = refAutoLock.m_pLock;
		}
		return *this;
	}

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

//自定义日志记录类
class CLog : public CSingle<CLog> {
public:
	CLog()
	{
		m_nLogLevel = enDEFAULT;
		m_nCurrentIndex = 0;
		m_pFp = NULL;
		ZeroMemory(m_szFilePath, MAX_PATH);
		ZeroMemory(m_szFileName, MAX_PATH);
		ZeroMemory(m_szLogRrefix, BASE_DATA_BUF_SIZE);
		ZeroMemory(m_szLogContent, BASE_DATA_BUF_SIZE);
	}
	~CLog()
	{
		m_nLogLevel = enDEFAULT;
		if (m_pFp) {
			fclose(m_pFp);
			m_pFp = NULL;
		}
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
		auto nFileSize = ftell(m_pFp);

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
			strcpy(szLoglevel, "WARNS");
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
			strcpy(szLoglevel, "INFOS");
			break;
		}
		sprintf_s(m_szLogRrefix, "%s[%5d][%s]:", m_szLogRrefix, ::GetCurrentThreadId(), szLoglevel);
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
	int			m_nLogLevel;
	int			m_nCurrentIndex;
	FILE*		m_pFp;
	CCritSec	m_csLock;
	char		m_szFilePath[MAX_PATH];
	char		m_szFileName[MAX_PATH];
	char		m_szLogRrefix[BASE_DATA_BUF_SIZE];
	char		m_szLogContent[BASE_DATA_BUF_SIZE];
	SYSTEMTIME	m_sysTime;
};

/*内存管理类*/
class CBuffer
{
public:
	CBuffer()
	{
		m_uiSize = 0;
		m_pBase = nullptr;
		m_pPtr = m_pBase;
	}
	virtual ~CBuffer()
	{
		ClearBuffer();
	}
	/*清理缓存数据*/
	void ClearBuffer()
	{
		SAFE_DELETE_ARRAY(m_pBase);
		m_pPtr = nullptr;
		m_uiSize = 0;
	}
	/*获取当前数据长度*/
	UINT GetBufferLen()
	{
		return (UINT)(m_pPtr - m_pBase);
	}
	/*读，返回读取的数据长度，bUpdate标志表示读的时候取出数据更新缓冲区*/
	UINT Read(PBYTE pData, UINT uiSize, BOOL bUpdate = TRUE)
	{
		UINT uiDataLen = GetBufferLen();
		if (uiDataLen <= 0 || !pData)
		{
			return 0;
		}
		else
		{
			if (uiSize > uiDataLen)
			{
				/*读取长度超出数据本身长度，不做处理*/
				return 0;
			}
			else
			{
				/*返回读取的数据长度*/
				memcpy(pData, m_pBase, uiSize);
				if(bUpdate)
				{
					/*如果需要，读取完数据前移*/
					memmove(m_pBase, m_pBase + uiSize, m_uiSize - uiSize);
					m_pPtr -= uiSize;
				}
				return uiSize;
			}
		}
	}
	/*写，返回写入的数据长度*/
	UINT Write(PBYTE pData, UINT uiSize)
	{
		if (uiSize <= 0 || !pData)
		{
			return 0;
		}
		else
		{
			UINT uiRemainLen = m_uiSize - GetBufferLen();
			if (uiRemainLen >= uiSize)
			{
				/*空间足够，直接拷贝内存*/
				memcpy(m_pPtr, pData, uiSize);
				m_pPtr += uiSize;
			}
			else
			{
				/*空间不足，重新申请内存*/
				UINT uiOriginLen = ReAllocateBuffer(uiSize);
				memcpy(m_pPtr, pData, uiSize);
				m_pPtr += uiSize;
			}
			return uiSize;
		}
	}
	/*获取缓存头指针*/
	PBYTE GetBuffer()
	{
		return m_pBase;
	}

protected:
	/*重新申请内存空间*/
	UINT ReAllocateBuffer(UINT uiRequestedSize)
	{
		UINT uiOriginLen = GetBufferLen();
		if (uiOriginLen >= 0)
		{
			PBYTE pTemp = new BYTE[uiOriginLen];
			memcpy(pTemp, m_pBase, uiOriginLen);
			SAFE_DELETE_ARRAY(m_pBase);

			UINT uiAllocateLen = m_uiSize + (int)(uiRequestedSize / BASE_CBUFFER_SIZE + 1) * BASE_CBUFFER_SIZE;
			m_pBase = new BYTE[uiAllocateLen];
			memcpy(m_pBase, pTemp, uiOriginLen);
			SAFE_DELETE_ARRAY(pTemp);

			m_pPtr = m_pBase + uiOriginLen;
			m_uiSize = uiAllocateLen;
			return uiOriginLen;
		}
		return 0;
	}
	/*释放多余内存空间*/
	UINT DeAllocateBuffer(UINT uiRequestedSize)
	{
		UINT uiOriginLen = GetBufferLen();
		if (uiOriginLen > 0)
		{
			/*释放的长度大于当前所有数据长度，清理缓存数据*/
			if (uiRequestedSize >= uiOriginLen)
			{
				ClearBuffer();
				return uiOriginLen;
			}
			else
			{
				PBYTE pTemp = new BYTE[uiOriginLen];
				memcpy(pTemp, m_pBase, uiOriginLen);
				SAFE_DELETE_ARRAY(m_pBase);

				UINT uiAllocateLen = (int)((uiOriginLen - uiRequestedSize) / BASE_CBUFFER_SIZE + 1)*BASE_CBUFFER_SIZE;
				m_pBase = new BYTE[uiAllocateLen];
				memcpy(m_pBase, pTemp + uiRequestedSize, uiOriginLen);
				SAFE_DELETE_ARRAY(pTemp);

				m_pPtr = m_pBase + uiOriginLen;
				m_uiSize = uiAllocateLen;
				return (uiAllocateLen - m_uiSize);
			}
		}
		return 0;
	}
	/*获取当前申请的内存空间*/
	UINT GetMemSize()
	{
		return m_uiSize;
	}

protected:
	PBYTE	m_pBase;
	PBYTE	m_pPtr;
	UINT	m_uiSize;
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
class CBufferPool {
public:
	CBufferPool()
	{
		m_dwBufferSize = 0;
		m_dwDataSize = 0;
		m_dwInsertPos = 0;
		m_dwTrailPos = 0;
		m_dwQueryPos = 0;
		m_dwPacketCount = 0;
		m_pDataBuffer = NULL;
	}
	virtual ~CBufferPool()
	{
		RemoveData();
	}

	virtual BOOL AddData(WORD wIdentifier, void* const pData, const WORD wDataLen)
	{
		if (!pData || 0 == wDataLen)
		{
			return FALSE;
		}

		DataHead stuDataHead = { 0 };
		stuDataHead.wIdentifier = wIdentifier;
		stuDataHead.wDataSize = wDataLen;
		
		// 需要添加的数据的长度
		DWORD dwInsertSize = sizeof(DataHead) + wDataLen;

		BOOL bResize = FALSE;
		if ((m_dwDataSize + dwInsertSize) > m_dwBufferSize)
		{
			// 数据总长度超出，需要重新分配内存
			bResize = TRUE;
		}
		else if ((m_dwInsertPos == m_dwTrailPos) && ((m_dwInsertPos + dwInsertSize) > m_dwBufferSize))
		{
			// 数据插入达到内存长度，此时判断当前内存区前半部已经消费过的内存长度，是否可以复用
			if ((m_dwQueryPos >= dwInsertSize))
			{
				// 如果长度符合要求，重新利用前面的内存区
				m_dwInsertPos = 0;
			}
			else
			{
				// 否则需要重新申请内存
				bResize = TRUE;
			}
		}
		else if ((m_dwInsertPos < m_dwTrailPos) && (m_dwInsertPos + dwInsertSize) > m_dwQueryPos)
		{
			// 复用已取出的内存区，接着添加数据判断空间是否足够
			bResize = TRUE;
		}
		
		// 内存区空间不足，需要重新申请内存
		if (bResize)
		{
			DWORD dwNewSize = m_dwBufferSize + (wDataLen + sizeof(DataHead)) * 10L;
			PBYTE pNewDataBuffer = new BYTE[dwNewSize];
			if (!pNewDataBuffer)
			{
				return FALSE;
			}
			DWORD dwOldDataSize = m_dwTrailPos - m_dwQueryPos;
			if (dwOldDataSize > 0)
			{
				// 取出后半段内存
				memcpy(pNewDataBuffer, m_pDataBuffer + m_dwQueryPos, dwOldDataSize);
			}
			if (dwOldDataSize < m_dwDataSize)
			{
				// 如果前半段存在复用数据，同样取出
				memcpy(pNewDataBuffer + dwOldDataSize, m_pDataBuffer, m_dwInsertPos);
			}

			// 更新内存区信息
			m_dwQueryPos = 0L;
			m_dwInsertPos = m_dwDataSize;
			m_dwTrailPos = m_dwDataSize;
			m_dwBufferSize = dwNewSize;
			SAFE_DELETE_ARRAY(m_pDataBuffer);
			m_pDataBuffer = pNewDataBuffer;
		}

		// 将数据copy到内存区
		memcpy(m_pDataBuffer + m_dwInsertPos, &stuDataHead, sizeof(DataHead));
		memcpy(m_pDataBuffer + m_dwInsertPos + sizeof(DataHead), pData, wDataLen);

		// 更新内存区信息
		m_dwPacketCount++;
		m_dwDataSize += dwInsertSize;
		m_dwInsertPos += dwInsertSize;
		// m_dwTrailPos始终保持不小于m_dwInsertPos
		m_dwTrailPos = max(m_dwTrailPos, m_dwInsertPos);

		return TRUE;
	}
	virtual BOOL GetData(DataHead& rDataHead, void* pData, WORD wDataLen)
	{
		if (!pData || 0 == wDataLen || 0 == m_dwPacketCount || 0 == m_dwDataSize)
		{
			return FALSE;
		}

		// 数据消费到数据尾部，将查询位重置为数据头部，更新数据尾部为当前数据插入位
		if (m_dwTrailPos == m_dwQueryPos)
		{
			m_dwQueryPos = 0L;
			m_dwTrailPos = m_dwInsertPos;
		}

		LPDataHead lpDataHead = (LPDataHead)(m_pDataBuffer + m_dwQueryPos);
		WORD wPacketDataLen = sizeof(DataHead) + lpDataHead->wDataSize;
		WORD wCopySize = 0;
		if (wDataLen >= lpDataHead->wDataSize)
		{
			wCopySize = lpDataHead->wDataSize;
		}

		rDataHead = *lpDataHead;
		if (rDataHead.wDataSize > 0)
		{
			if (wDataLen < lpDataHead->wDataSize)
			{
				rDataHead.wDataSize = 0;
			}
			else
			{
				memcpy(pData, lpDataHead + 1, rDataHead.wDataSize);
			}
		}

		// 更新内存区参数
		m_dwPacketCount--;
		m_dwDataSize -= wPacketDataLen;
		m_dwQueryPos += wPacketDataLen;

		return TRUE;
	}

	virtual void GetBufferPoolInfo(BUFFER_INFO& rBufferInfo)
	{
		rBufferInfo.dwDataSize = m_dwDataSize;
		rBufferInfo.dwBufferSize = m_dwBufferSize;
		rBufferInfo.dwDataPacketCount = m_dwPacketCount;
	}
	virtual void RemoveData()
	{
		m_dwBufferSize = 0;
		m_dwDataSize = 0;
		m_dwInsertPos = 0;
		m_dwTrailPos = 0;
		m_dwQueryPos = 0;
		m_dwPacketCount = 0;
		SAFE_DELETE_ARRAY(m_pDataBuffer);
	}

private:
	DWORD m_dwBufferSize;     // 内存区长度
	DWORD m_dwDataSize;       // 内存区内数据长度
	DWORD m_dwInsertPos;      // 数据插入位
	DWORD m_dwTrailPos;       // 数据结束位
	DWORD m_dwQueryPos;       // 数据查询位
	DWORD m_dwPacketCount;    // 内存区数据包数量
	PBYTE m_pDataBuffer;      // 内存区数据指针
};
class CRingBuffer {
public:
	CRingBuffer(int nSize)
	{
		m_nBufferSize = nSize;
		m_pBuffer = new char[m_nBufferSize];
		Clear();
	}
	~CRingBuffer()
	{
		delete[] m_pBuffer;
		m_pBuffer = NULL;
		Clear();
	}

public:
	void Clear()
	{
		m_nWritePos = 0;
		m_nReadPos = 0;
		m_nCurrentBufferSize = 0;
	}
	bool Full() const
	{
		return (m_nCurrentBufferSize == m_nBufferSize);
	}
	bool Empty() const
	{
		return (0 == m_nCurrentBufferSize);
	}
	int	GetLength() const
	{
		return m_nCurrentBufferSize;
	}

	int Write(char* pDataPtr, int nDataLen)
	{
		if (nDataLen <= 0 || Full())
		{
			return 0;
		}

		CAutoLock lock(&m_csLock);
		if (m_nReadPos == m_nWritePos && 0 == m_nCurrentBufferSize)
		{
			int nLeftSize = m_nBufferSize - m_nWritePos;
			if (nLeftSize >= nDataLen)
			{
				memcpy(m_pBuffer + m_nWritePos, pDataPtr, nDataLen);
				m_nWritePos += nDataLen;
				m_nCurrentBufferSize += nDataLen;
				return nDataLen;
			}
			else
			{
				assert(nLeftSize >= nDataLen);
				return 0;
			}
		}
		else if (m_nReadPos < m_nWritePos)
		{
			int nLeftSize = m_nBufferSize - (m_nWritePos - m_nReadPos);
			int nBehindSize = m_nBufferSize - m_nWritePos;
			if (nLeftSize >= nDataLen)
			{
				if (nBehindSize >= nDataLen)
				{
					memcpy(m_pBuffer + m_nWritePos, pDataPtr, nDataLen);
					m_nWritePos += nDataLen;
					m_nCurrentBufferSize += nDataLen;
					return nDataLen;
				}
				else
				{
					memcpy(m_pBuffer + m_nWritePos, pDataPtr, nBehindSize);
					memcpy(m_pBuffer, pDataPtr + nBehindSize, nDataLen - nBehindSize);
					m_nWritePos = nDataLen - nBehindSize;
					m_nCurrentBufferSize += nDataLen;
					return nDataLen;
				}
			}
			else
			{
				assert(nLeftSize >= nDataLen);
				return 0;
			}
		}
		else
		{
			int nLeftSize = m_nReadPos - m_nWritePos;
			if (nLeftSize >= nDataLen)
			{
				memcpy(m_pBuffer + m_nWritePos, pDataPtr, nDataLen);
				m_nWritePos += nDataLen;
				return nDataLen;
			}
			else
			{
				assert(nLeftSize >= nDataLen);
				return 0;
			}
		}
	}
	int Read(char* pDataPtr, int nDataLen)
	{
		if (nDataLen <= 0 || Empty())
		{
			return 0;
		}

		CAutoLock lock(&m_csLock);
		if (m_nReadPos == m_nWritePos && m_nBufferSize == m_nCurrentBufferSize)
		{
			return 0;
		}
		else if (m_nReadPos < m_nWritePos)
		{
			int nBufferSize = m_nWritePos - m_nReadPos;
			if (nBufferSize >= nDataLen)
			{
				memcpy(pDataPtr, m_pBuffer + m_nReadPos, nDataLen);
				memset(m_pBuffer + m_nReadPos, 0, nDataLen);
				m_nReadPos += nDataLen;
				m_nCurrentBufferSize -= nDataLen;
				return nDataLen;
			}
			else
			{
				assert(nBufferSize >= nDataLen);
				return 0;
			}
		}
		else
		{
			int nBufferSize = m_nBufferSize - (m_nReadPos - m_nWritePos);
			int nBehindSize = m_nBufferSize - m_nReadPos;
			if (nBufferSize >= nDataLen)
			{
				if (nBehindSize >= nDataLen)
				{
					memcpy(pDataPtr, m_pBuffer + m_nReadPos, nDataLen);
					memset(m_pBuffer + m_nReadPos, 0, nDataLen);
					m_nReadPos += nDataLen;
					m_nCurrentBufferSize -= nDataLen;
					return nDataLen;
				}
				else
				{
					memcpy(pDataPtr, m_pBuffer + m_nReadPos, m_nBufferSize - m_nReadPos);
					memset(m_pBuffer + m_nReadPos, 0, m_nBufferSize - m_nReadPos);
					memcpy(pDataPtr + (m_nBufferSize - m_nReadPos), m_pBuffer, nDataLen - (m_nBufferSize - m_nReadPos));
					memset(m_pBuffer, 0, nDataLen - (m_nBufferSize - m_nReadPos));
					m_nReadPos = nDataLen - (m_nBufferSize - m_nReadPos);
					m_nCurrentBufferSize -= nDataLen;
					return nDataLen;
				}
			}
			else
			{
				assert(nBufferSize >= nDataLen);
				return 0;
			}
		}
	}

private:
	CCritSec	m_csLock;
	char*		m_pBuffer;
	int			m_nWritePos;
	int			m_nReadPos;
	int			m_nCurrentBufferSize;
	int			m_nBufferSize;
};