#pragma once

#include <sstream>
#include <deque>
#include <map>
#include <thread>
#include <regex>
#include <functional>
#include <condition_variable>
#include <process.h>
#include <ws2tcpip.h>
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")
#include <MSWSock.h>

#include "../Common/google/protobuf/message.h"
#if _DEBUG
#pragma comment(lib,"libprotobufd.lib")
#pragma comment(lib,"libprotocd.lib")
#else
#endif

#include "def.h"
#include "util.h"
#include "lock.h"
#include "data.h"
#include "log.h"

using namespace std;

enum class IoType {
	enIoInit = 0,
	enIoAccept = 1,
	enIoWrite = 2,
	enIoRead = 3,
	enIoClose = 4,
	enIoConn = 5,
};

class CSocketContext;
class NetPacketHead {
public:
	int nDataLen;
};
class NetPacket {
public:
	NetPacket(CSocketContext* pContext, IDataBuffer* pDataBuffer)
	{
		m_pContext = pContext;
		m_pDataBuffer = pDataBuffer;
	}
	~NetPacket()
	{

	}
	NetPacket(const NetPacket&) = delete;
	NetPacket& operator=(const NetPacket&) = delete;

public:
	CSocketContext* GetCtx()
	{
		return m_pContext;
	}
	IDataBuffer* GetBuffer()
	{
		return m_pDataBuffer;
	}
	BYTE* GetData()
	{
		return (BYTE*)(m_pDataBuffer->GetData());
	}
	UINT GetDataLen()
	{
		return m_pDataBuffer->GetDataLen();
	}
	void Release()
	{
		m_pContext = nullptr;
		m_pDataBuffer->Release();
		m_pDataBuffer = nullptr;
	}

private:
	CSocketContext* m_pContext;
	IDataBuffer* m_pDataBuffer;
};
struct ContextHead {
	SOCKET hSocket;
	ULONG lToken;
};
struct RequestHead {
	int nRepeated;
	int nRequest;
	int nSubRequest;
};
struct NetRequest {
	RequestHead head;
	int nDataLen;
	void* pData;
};

class CSocketBuffer {
public:
	WSAOVERLAPPED		m_ol;
	SOCKET				m_hSocket;
	IoType				m_ioType;
	LONGLONG			m_llSerialNo;
	IDataBuffer*		m_pBuffer;
	CSocketBuffer*		m_pNext;

public:
	CSocketBuffer()
	{
		ZeroMemory(&m_ol, sizeof(m_ol));
		m_hSocket = INVALID_SOCKET;
		m_ioType = IoType::enIoInit;
		m_pBuffer = nullptr;
		m_pNext = nullptr;
	}
	virtual ~CSocketBuffer()
	{
		ZeroMemory(&m_ol, sizeof(m_ol));
		m_hSocket = INVALID_SOCKET;
		m_ioType = IoType::enIoInit;
		m_pNext = nullptr;
	}
	CSocketBuffer(const CSocketBuffer&) = delete;
	CSocketBuffer& operator=(const CSocketBuffer&) = delete;
};
class CSocketBufferMgr {
public:
	CSocketBufferMgr()
	{
		m_pSocketBufferList = nullptr;
		m_pAllocator = nullptr;
		m_pPendingAcceptList = nullptr;
	}
	virtual ~CSocketBufferMgr()
	{
		m_pSocketBufferList = nullptr;
	}
	CSocketBufferMgr(const CSocketBufferMgr&) = delete;
	CSocketBufferMgr& operator=(const CSocketBufferMgr&) = delete;

public:
	bool Init(CDataBufferMgr* pAllocator)
	{
		m_pAllocator = pAllocator;
		return true;
	}
	CDataBufferMgr* GetAllocator()
	{
		return m_pAllocator;
	}
	CSocketBuffer* AllocateSocketBuffer(UINT nBufferLen)
	{
		if (nBufferLen <= 0)
		{
			return nullptr;
		}
		CSocketBuffer* pBuffer = nullptr;
		{
			CAutoLock lock(&m_lock);
			if (m_pSocketBufferList)
			{
				pBuffer = m_pSocketBufferList;
				m_pSocketBufferList = m_pSocketBufferList->m_pNext;
				pBuffer->m_pNext = nullptr;
			}
			else
			{
				pBuffer = new CSocketBuffer();
			}
		}
		if (pBuffer)
		{
			pBuffer->m_pBuffer = m_pAllocator->AllocateDataBuffer(nBufferLen);
			if (pBuffer->m_pBuffer)
			{
				pBuffer->m_pBuffer->SetBufferLen(nBufferLen);
			}
			else
			{
				return nullptr;
			}
			return pBuffer;
		}
		else {
			return nullptr;
		}
	}
	void ReleaseSocketBuffer(CSocketBuffer* pBuffer)
	{
		if (!pBuffer)
		{
			return;
		}
		ZeroMemory(&pBuffer->m_ol, sizeof(pBuffer->m_ol));
		pBuffer->m_hSocket = INVALID_SOCKET;
		pBuffer->m_ioType = IoType::enIoInit;
		pBuffer->m_llSerialNo = 0;
		if (pBuffer->m_pBuffer)
		{
			pBuffer->m_pBuffer->Release();
			pBuffer->m_pBuffer = nullptr;
		}
		pBuffer->m_pNext = nullptr;
		{
			CAutoLock lock(&m_lock);
			pBuffer->m_pNext = m_pSocketBufferList;
			m_pSocketBufferList = pBuffer;
		}
	}
	void FreeSocketBuffer()
	{
		CAutoLock lock(&m_lock);
		CSocketBuffer* pTemper = nullptr;
		CSocketBuffer* pBuffer = m_pSocketBufferList;
		while (pBuffer)
		{
			pTemper = pBuffer->m_pNext;
			delete pBuffer;
			pBuffer = pTemper;
		}
		m_pSocketBufferList = nullptr;
	}
	void InsertPendingAccepts(CSocketBuffer* pBuffer)
	{
		CAutoLock lock(&m_lock);
		if (!m_pPendingAcceptList)
		{
			m_pPendingAcceptList = pBuffer;
		}
		else
		{
			pBuffer->m_pNext = m_pPendingAcceptList;
			m_pPendingAcceptList = pBuffer;
		}
	}
	void RemovePendingAccepts(CSocketBuffer* pBuffer)
	{
		CAutoLock lock(&m_lock);
		CSocketBuffer* pTemp = m_pPendingAcceptList;
		if (pTemp == pBuffer)
		{
			m_pPendingAcceptList = pBuffer->m_pNext;
		}
		else
		{
			while (pTemp && pTemp->m_pNext != pBuffer)
			{
				pTemp = pTemp->m_pNext;
			}
			if (pTemp)
			{
				pTemp->m_pNext = pBuffer->m_pNext;
				pBuffer->m_pNext = nullptr;
			}
		}
	}
	void CheckPendingAccpets()
	{
		int nTimes = 0;
		int nTimesLen = sizeof(int);
		CAutoLock lock(&m_lock);
		CSocketBuffer* pBuffer = m_pPendingAcceptList;
		while (pBuffer)
		{
			// 获取socket连接建立时长，时间长的话直接断开
			if (pBuffer->m_hSocket != INVALID_SOCKET)
			{
				::getsockopt(pBuffer->m_hSocket, SOL_SOCKET, SO_CONNECT_TIME, (char*)& nTimes, &nTimesLen);
				if (nTimes <= 2)
				{
					pBuffer = pBuffer->m_pNext;
					continue;
				}
			}
			CSocketBuffer* pDelete = pBuffer;
			pBuffer = pBuffer->m_pNext;
			RemovePendingAccepts(pDelete);
			ReleaseSocketBuffer(pDelete);
		}
	}

private:
	CSocketBuffer*		m_pSocketBufferList;
	CSocketBuffer*		m_pPendingAcceptList;
	CDataBufferMgr*		m_pAllocator;
	CCritSec			m_lock;
};

class CIocpTcpServer;
class CSocketContextMgr;
class CSocketContext {
public:
	SOCKET						m_hSocket;
	ULONG						m_lToken;
	UINT						m_nFlag;
	BOOL						m_bClosing;
	BOOL						m_bDelayClose;

	SOCKADDR_IN					m_local;
	SOCKADDR_IN					m_remote;

	BYTE						m_recvDataBuff[8192];
	UINT						m_nRecvDataLen;
	LONGLONG					m_llPendingRecvs;
	CSocketBuffer*				m_pWaitingRecv;

	LONGLONG					m_llPendingSends;
	CSocketBuffer*				m_pWaitingSend;

	LONGLONG					m_llCurrSerialNo;
	LONGLONG					m_llNextSerialNo;

	CSocketContext*				m_pNext;
	CSocketContextMgr*			m_pCtxManager;
	CSocketBufferMgr*			m_pBufManager;
	CDataBufferMgr*				m_pAllocator;

	CCritSec					m_lock;

public:
	CSocketContext()
	{
		m_hSocket = INVALID_SOCKET;
		m_lToken = 0L;
		m_nFlag = 0;
		m_bClosing = FALSE;
		m_bDelayClose = FALSE;
		ZeroMemory(&m_local, sizeof(m_local));
		ZeroMemory(&m_remote, sizeof(m_remote));
		ZeroMemory(&m_recvDataBuff, 8192);
		m_nRecvDataLen = 0;
		m_llPendingRecvs = 0;
		m_pWaitingRecv = nullptr;
		m_llCurrSerialNo = 0;
		m_llNextSerialNo = 0;
		m_llPendingSends = 0;
		m_pWaitingSend = nullptr;
		m_pNext = nullptr;
		m_pCtxManager = nullptr;
		m_pBufManager = nullptr;
		m_pAllocator = nullptr;
	}
	virtual ~CSocketContext()
	{
		m_hSocket = INVALID_SOCKET;
		m_lToken = 0L;
		m_nFlag = 0;
		m_bClosing = FALSE;
		m_bDelayClose = FALSE;
		ZeroMemory(&m_local, sizeof(m_local));
		ZeroMemory(&m_remote, sizeof(m_remote));
		ZeroMemory(&m_recvDataBuff, 8192);
		m_nRecvDataLen = 0;
		m_llPendingRecvs = 0;
		m_pWaitingRecv = nullptr;
		m_llCurrSerialNo = 0;
		m_llNextSerialNo = 0;
		m_llPendingSends = 0;
		m_pWaitingSend = nullptr;
		m_pNext = nullptr;
		m_pCtxManager = nullptr;
		m_pBufManager = nullptr;
		m_pAllocator = nullptr;
	}
	CSocketContext(const CSocketContext&) = delete;
	CSocketContext& operator=(const CSocketContext&) = delete;

public:
	bool HandleRecvData(CSocketBuffer* pBuffer, DWORD dwTrans, CIocpTcpServer* pServer);
	bool CheckHeader(NetPacketHead* pData);
	LONGLONG GetPendingRecvs()
	{
		CAutoLock lock(&m_lock);
		return m_llPendingRecvs;
	}
};
class CSocketListenContext {
public:
	SOCKET						m_hSocket;
	HANDLE						m_hAcceptHandle;
	HANDLE						m_hRepostHandle;
	UINT						m_nRepostCount;
	LPFN_ACCEPTEX				m_lpfnAcceptEx;
	LPFN_GETACCEPTEXSOCKADDRS	m_lpfnGetAcceptExSockaddrs;

	CSocketListenContext*		m_pNext;

public:
	CSocketListenContext()
	{
		m_hSocket = INVALID_SOCKET;
		m_hAcceptHandle = INVALID_HANDLE_VALUE;
		m_hRepostHandle = INVALID_HANDLE_VALUE;
		m_lpfnAcceptEx = nullptr;
		m_lpfnGetAcceptExSockaddrs = nullptr;
		m_pNext = nullptr;
		m_nRepostCount = 0;
	}
	~CSocketListenContext()
	{
		m_lpfnAcceptEx = nullptr;
		m_lpfnGetAcceptExSockaddrs = nullptr;
		m_hSocket = INVALID_SOCKET;
		m_hAcceptHandle = INVALID_HANDLE_VALUE;
		m_hRepostHandle = INVALID_HANDLE_VALUE;
		m_nRepostCount = 0;
	}

	bool Init()
	{
		m_hAcceptHandle = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		if (INVALID_HANDLE_VALUE == m_hAcceptHandle)
		{
			myLogConsoleE("%s 句柄m_hAcceptHandle无效", __FUNCTION__);
			return false;
		}

		m_hRepostHandle = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		if (INVALID_HANDLE_VALUE == m_hRepostHandle)
		{
			myLogConsoleE("%s 句柄m_hRepostHandle无效", __FUNCTION__);
			return false;
		}

		m_hSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (INVALID_SOCKET == m_hSocket)
		{
			myLogConsoleE("%s 句柄m_hSocket无效", __FUNCTION__);
			return false;
		}

		DWORD bytes = 0;
		GUID guid = WSAID_ACCEPTEX;
		::WSAIoctl(m_hSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, (LPVOID)&guid, sizeof(guid), &(m_lpfnAcceptEx), sizeof(m_lpfnAcceptEx), &bytes, NULL, NULL);
		if (!m_lpfnAcceptEx)
		{
			myLogConsoleE("%s m_lpfnAcceptEx为nullptr!!!", __FUNCTION__, m_hSocket);
			return false;
		}
		guid = WSAID_GETACCEPTEXSOCKADDRS;
		::WSAIoctl(m_hSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, (LPVOID)&guid, sizeof(guid), &(m_lpfnGetAcceptExSockaddrs), sizeof(m_lpfnGetAcceptExSockaddrs), &bytes, NULL, NULL);
		if (!m_lpfnGetAcceptExSockaddrs)
		{
			myLogConsoleE("%s m_lpfnGetAcceptExSockaddrs为nullptr!!!", __FUNCTION__, m_hSocket);
			return false;
		}

		return true;
	}
	void NotifyRepostAccepts()
	{
		::InterlockedIncrement(&m_nRepostCount);
		::SetEvent(m_hRepostHandle);
	}
};
class CSocketContextMgr {
public:
	CSocketContextMgr()
	{
		m_lNextTokenID = 0L;
		m_lMaxConnections = 0L;
		m_pSocketContextList = nullptr;
		m_pListenContext = nullptr;
		m_pBufManager = nullptr;
		m_pAllocator = nullptr;
	}
	virtual ~CSocketContextMgr()
	{

	}
	CSocketContextMgr(const CSocketContextMgr&) = delete;
	CSocketContextMgr& operator=(const CSocketContextMgr&) = delete;

public:
	CSocketContext* AllocateSocketContext(SOCKET hSocket, UINT nIndex = 0)
	{
		CSocketContext* pContext = nullptr;
		if (INVALID_SOCKET == hSocket)
		{
			myLogConsoleE("%s 参数错误(INVALID_SOCKET == hSocket)", __FUNCTION__);
			return pContext;
		}

		CAutoLock lock(&m_lock);
		ULONG lToken = CreateTokenID();
		if (ULONG_MAX == lToken)
		{
			myLogConsoleW("%s 达到规定最多连接限制数%ld 当前连接数量%d", __FUNCTION__, m_lMaxConnections, m_mapConnection.size());
			return pContext;
		}
		if (m_pSocketContextList)
		{
			pContext = m_pSocketContextList;
			m_pSocketContextList = m_pSocketContextList->m_pNext;
			pContext->m_pNext = nullptr;
		}
		else
		{
			pContext = new CSocketContext();
			pContext->m_pCtxManager = this;
			pContext->m_pBufManager = m_pBufManager;
			pContext->m_pAllocator = m_pAllocator;
		}
		if (pContext)
		{
			pContext->m_bClosing = FALSE;
			pContext->m_bDelayClose = FALSE;
			pContext->m_hSocket = hSocket;
			pContext->m_lToken = lToken;
			pContext->m_nFlag = nIndex;
			if (nIndex > 0)
			{
				m_mapServer.insert(std::make_pair(nIndex, pContext->m_hSocket));
			}
			m_mapConnection.insert(std::make_pair(hSocket, pContext));
			return pContext;
		}
		else
		{
			SAFE_RELEASE_SOCKET(hSocket);
			return nullptr;
		}
	}
	void ReleaseSocketContext(CSocketContext* pContext)
	{
		CAutoLock lock(&m_lock);
		if (!pContext)
		{
			myLogConsoleE("%s 参数错误(NULL == pContext)", __FUNCTION__);
			return;
		}
		ZeroMemory(&pContext->m_local, sizeof(pContext->m_local));
		ZeroMemory(&pContext->m_remote, sizeof(pContext->m_remote));
		ZeroMemory(&pContext->m_recvDataBuff, 8192);
		pContext->m_nRecvDataLen = 0;
		pContext->m_llPendingRecvs = 0;
		pContext->m_pWaitingRecv = nullptr;
		pContext->m_llCurrSerialNo = 0;
		pContext->m_llNextSerialNo = 0;
		pContext->m_llPendingSends = 0;
		pContext->m_pNext = m_pSocketContextList;
		m_pSocketContextList = pContext;
		if (m_mapConnection.find(pContext->m_hSocket) != m_mapConnection.end())
		{
			m_mapConnection.erase(pContext->m_hSocket);
		}
		if (m_mapServer.find(pContext->m_nFlag) != m_mapServer.end())
		{
			m_mapServer.erase(pContext->m_nFlag);
		}
		pContext->m_nFlag = 0;
		m_listFreeTokens.emplace_back(pContext->m_lToken);
		pContext->m_lToken = 0L;
		SAFE_RELEASE_SOCKET(pContext->m_hSocket);
	}
	void FreeSocketContext()
	{
		CAutoLock lock(&m_lock);
		CSocketContext* pTemper = nullptr;
		CSocketContext* pContext = m_pSocketContextList;
		while (pContext)
		{
			pTemper = pContext->m_pNext;
			delete pContext;
			pContext = pTemper;
		}
		m_pSocketContextList = nullptr;
		m_mapConnection.clear();
		m_listFreeTokens.clear();
	}
	void InsertPendingCloses(CSocketContext* pContext)
	{
		if (INVALID_SOCKET == pContext->m_hSocket)
		{
			return;
		}
		CAutoLock lock(&m_lockClose);
		if (m_mapPendingCloses.find(pContext->m_hSocket) == m_mapPendingCloses.end())
		{
			m_mapPendingCloses.insert(std::make_pair(pContext->m_hSocket, GetTickCount64()));
		}
	}
	void RemovePendingCloses(SOCKET hSocket)
	{
		if (INVALID_SOCKET == hSocket)
		{
			return;
		}
		CAutoLock lock(&m_lockClose);
		if (m_mapPendingCloses.find(hSocket) != m_mapPendingCloses.end())
		{
			m_mapPendingCloses.erase(hSocket);
		}
	}
	void CheckPendingCloses()
	{
		ULONGLONG tick = GetTickCount64();
		CAutoLock lock(&m_lockClose);
		for (auto iter = m_mapPendingCloses.begin(); iter != m_mapPendingCloses.end();)
		{
			auto pContext = GetCtx(iter->first);
			if (pContext)
			{
				if ((tick - iter->second) > 5000 && pContext->GetPendingRecvs() <= 1)
				{
					ReleaseSocketContext(pContext);
					m_mapPendingCloses.erase(iter++);
				}
				else
				{
					iter++;
				}
			}
			else
			{
				iter++;
			}
		}
	}

	bool Init(CDataBufferMgr* pAllocator, CSocketBufferMgr* pBufManager, ULONG nMaxConnections)
	{
		m_pListenContext = new CSocketListenContext();
		if (!m_pListenContext || !m_pListenContext->Init())
		{
			myLogConsoleE("%s CSocketContextMgr初始化失败", __FUNCTION__);
			return false;
		}
		m_lMaxConnections = nMaxConnections;
		m_pAllocator = pAllocator;
		m_pBufManager = pBufManager;
		return true;
	}
	CSocketContext* GetCtx(SOCKET hSocket)
	{
		CSocketContext* pContext = nullptr;
		CAutoLock lock(&m_lock);
		auto iter = m_mapConnection.find(hSocket);
		if (iter != m_mapConnection.end())
		{
			pContext = iter->second;
		}
		return pContext;
	}
	CSocketListenContext* GetListenCtx()
	{
		return m_pListenContext;
	}
	SOCKET GetSocket(UINT nIndex)
	{
		CSocketContext* pContext = nullptr;
		CAutoLock lock(&m_lock);
		auto iter = m_mapServer.find(nIndex);
		if (iter != m_mapServer.end())
		{
			return iter->second;
		}
		return INVALID_SOCKET;
	}
	SOCKET GetListenSocket()
	{
		return m_pListenContext->m_hSocket;
	}
	HANDLE GetAcceptHandle()
	{
		return m_pListenContext->m_hAcceptHandle;
	}
	HANDLE GetRepostHandle()
	{
		return m_pListenContext->m_hRepostHandle;
	}
	void GetSockets(std::map<UINT, SOCKET>& mapServer)
	{
		CAutoLock lock(&m_lock);
		for (auto& iter : m_mapServer) {
			mapServer.insert(std::make_pair(iter.first, iter.second));
		}
	}

private:
	ULONG CreateTokenID()
	{
		ULONG lToken = ULONG_MAX;
		if (m_lNextTokenID < m_lMaxConnections)
		{
			lToken = m_lNextTokenID++;
		}
		else if (!m_listFreeTokens.empty())
		{
			lToken = m_listFreeTokens.front();
			m_listFreeTokens.pop_front();
		}
		return lToken;
	}

private:
	ULONG								m_lNextTokenID;
	ULONG								m_lMaxConnections;
	std::list<ULONG>					m_listFreeTokens;
	std::map<UINT, SOCKET>				m_mapServer;
	std::map<SOCKET, ULONGLONG>			m_mapPendingCloses;
	std::map<SOCKET, CSocketContext*>	m_mapConnection;
	CSocketContext*						m_pSocketContextList;
	CSocketListenContext*				m_pListenContext;
	CSocketBufferMgr*					m_pBufManager;
	CDataBufferMgr*						m_pAllocator;
	CCritSec							m_lock;
	CCritSec							m_lockClose;
};

class CThreadContext {
public:
	CThreadContext()
	{
		m_pContext = nullptr;
	}
	virtual ~CThreadContext()
	{

	}

	void* GetWorkerContext()
	{
		return m_pContext;
	}

public:
	std::thread m_thread;
	void* m_pContext;
};
class CIocpTcpServer {
public:
	CIocpTcpServer(DWORD dwFlag);
	virtual ~CIocpTcpServer();
	CIocpTcpServer(const CIocpTcpServer&) = delete;
	CIocpTcpServer& operator=(const CIocpTcpServer&) = delete;

	virtual bool InitializeMembers(UINT nMaxConnections);
	virtual bool BeginBindListen(const char* lpSzIp, UINT nPort, UINT nInitAccepts, UINT nMaxAccpets);
	virtual bool BeginThreadPool(UINT nThreads = 0);
	virtual bool InitializeIo();
	virtual bool Initialize(const char* lpSzIp, UINT nPort, UINT nInitAccepts, UINT nMaxAccpets, UINT nThreads, UINT nMaxConnections);
	virtual bool Shutdown();
	virtual bool EndWorkerPool();

	virtual bool PostAccept(CSocketContext* pContext, CSocketBuffer* pBuffer, DWORD& dwWSAError);
	virtual bool PostRecv(CSocketContext* pContext, CSocketBuffer* pBuffer, DWORD& dwWSAError);
	virtual bool PostSend(CSocketContext* pContext, CSocketBuffer* pBuffer, DWORD& dwWSAError);
	virtual bool PostConn(CSocketContext* pContext, CSocketBuffer* pBuffer, DWORD& dwWSAError);
	virtual bool PostClose(CSocketContext* pContext, CSocketBuffer* pBuffer, DWORD& dwWSAError);

	virtual bool CloseClient(SOCKET hSocket);
	virtual bool SendData(SOCKET hSocket, const void* pDataPtr, int nDataLen);
	virtual bool SendData(CSocketContext* pContext, const void* pDataPtr, int nDataLen);
	virtual bool SendPbData(CSocketContext* pContext, const google::protobuf::Message& pdata);
	virtual bool SendRequest(SOCKET hSocket, ContextHead* pContextHead, NetRequest* pRequest);

	virtual void HandleIo(DWORD dwKey, CSocketBuffer* pBuffer, DWORD dwTrans, DWORD dwError);
	virtual void HandleIoDefault(DWORD dwKey, CSocketBuffer* pBuffer, DWORD dwTrans, DWORD dwError);
	virtual void HandleIoAccept(DWORD dwKey, CSocketBuffer* pBuffer, DWORD dwTrans, DWORD dwError);
	virtual void HandleIoRead(DWORD dwKey, CSocketBuffer* pBuffer, DWORD dwTrans, DWORD dwError);
	virtual void HandleIoWrite(DWORD dwKey, CSocketBuffer* pBuffer, DWORD dwTrans, DWORD dwError);
	virtual void HandleIoClose(DWORD dwKey, CSocketBuffer* pBuffer, DWORD dwTrans, DWORD dwError);
	virtual void HandleIoConn(DWORD dwKey, CSocketBuffer* pBuffer, DWORD dwTrans, DWORD dwError);

	virtual void OnDataHandle(CSocketContext* pContext, IDataBuffer* pBuffer);
	virtual void DispatchData(NetPacket* pNP);
	virtual void OnRequest(void* p1, void* p2);

	virtual void* GetWorkerContext();
	virtual void* OnWorkerStart();
	virtual void OnWorkerExit(void* pContext);

protected:
	void AcceptThreadFunc();
	void SocketThreadFunc();
	void WorkerThreadFunc();

public:
	CDataBufferMgr*								m_pAllocator;
	CSocketBufferMgr*							m_pSocketBufferMgr;
	CSocketContextMgr*							m_pSocketContextMgr;

	TCHAR										m_szIp[16];
	UINT										m_nPort;
	UINT										m_nInitAccepts;
	DWORD										m_nFlag;

	HANDLE										m_hCompletionPort;
	HANDLE										m_hWaitHandle[2];

	std::thread									m_acceptThread;
	std::thread									m_socketThread[32];

	std::deque<NetPacket>						m_recvPacket;
	std::thread									m_dispatchThread;
	std::mutex									m_lock;
	std::condition_variable						m_cond;
	std::map<DWORD, CThreadContext*>			m_mapWorkerThreadCtx;

	bool										m_bShutdown;
};
class CIocpTcpClient : public CIocpTcpServer
{
public:
	CIocpTcpClient();
	virtual ~CIocpTcpClient();
	CIocpTcpClient(const CIocpTcpClient&) = delete;
	CIocpTcpClient& operator=(const CIocpTcpClient&) = delete;

public:
	virtual bool Create();
	virtual bool BeginThreadPool(UINT nThreads = 0);
	virtual bool ConnectOneServer(const std::string strIp, const int nPort, const UINT& nIndex);
	virtual bool BeginConnect(const std::string& strIp, const int& nPort, const UINT& nIndex);
	virtual bool DisconnectServer(const UINT& nIndex);
	virtual bool Destroy();

	virtual bool SendCast(const void* pDataPtr, const int& nDataLen);
	virtual bool SendOneServer(const UINT& nIndex, const void* pDataPtr, const int& nDataLen);
	virtual void OnRequest(void* p1, void* p2);
};
