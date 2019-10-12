#pragma once

typedef enum IoType {
	enIoInit,
	enIoAccept,
	enIoWrite,
	enIoRead,
	enIoDisconnect,
};

typedef enum MsgType {
	enGreeting,
	enRequest,
	enResponse,
	enAcknowledge,
};

// 工作队列数据参数
class WORKER_OV
{
public:
	OVERLAPPED	m_ol;
	void*		m_p1;
	void*		m_p2;

	WORKER_OV(void* p1 = NULL, void* p2 = NULL)
	{
		memset(this, 0, sizeof(WORKER_OV));
		m_p1 = p1;
		m_p2 = p2;
	}
};
// 自定义工作队列（基于完成端口）
class CIocpWorker {
public:
	CIocpWorker();
	virtual ~CIocpWorker();

	virtual BOOL BeginWorkerPool(int nThreads, int nConcurrency);
	virtual BOOL EndWorkerPool();

	virtual BOOL DoWorkLoop();
	virtual BOOL PutRequestToQueue(DWORD dwSize, DWORD dwKey, void* pParam1, void* pParam2);
	virtual BOOL PutDataToQueue(DWORD dwSize, DWORD dwKey, WORKER_OV* pWorkerOv);
	virtual BOOL OnRequest(void* pParam1, void* pParam2);

	virtual WORKER_OV* AllocateWorkerOv(void* pParam1, void* pParam2);
	virtual WORKER_OV* MoveWorkerOvToFree(WORKER_OV* pWorkerOv);

	virtual UINT GetWorkerCount();

	virtual void ClearWorkerOvLists();
	virtual void CloseWorkerHandles();

	static unsigned __stdcall WorkerThreadFunc(LPVOID lpParam);

public:
	CCritSec m_csWorkers;
	std::map<UINT, LPVOID> m_mapWorkers;

	CCritSec m_csWorkerOvList;
	std::list<LPVOID> m_listWorkerOv;
	std::list<LPVOID> m_listFreeWorkerOv;

	UINT m_nWorkerOvCount;
	UINT m_nFreeWorkerOvCount;
};

class CSocketBuffer {
public:
	WSAOVERLAPPED  m_ol;
	SOCKET         m_hSocket;
	LONG           m_lToken;
	BYTE*          m_pBuffer;
	UINT           m_nBufferLen;
	IoType         m_ioType;
	UINT           m_nSerialNo;

	CSocketBuffer* m_pNext;

public:
	CSocketBuffer()
	{
		m_pBuffer = NULL;
		m_pNext = NULL;
		m_ioType = enIoInit;
	}
	virtual ~CSocketBuffer()
	{
		if (m_pBuffer)
		{
			SAFE_DELETE(m_pBuffer);
		}
		if (m_pNext)
		{
			SAFE_DELETE(m_pNext);
		}
	}
};

class CSocketContext {
public:
	SOCKET            m_hSocket;
	LONG              m_lToken;

	SOCKADDR_IN       m_local;
	SOCKADDR_IN       m_remote;

	BOOL              m_bClose;

	UINT              m_nNextReadSerialNo;
	UINT              m_nCurrentReadSerialNo;
	UINT              m_nPostedSend;
	UINT              m_nPostedRecv;

	UINT              m_nPendingRecv;
	CSocketBuffer*    m_pWaitingRecv;
	UINT              m_nPendingSend;
	CSocketBuffer*    m_pWaitingSend;

	UINT              m_nPacketNo;
	UINT              m_nSessionID;
	CBuffer           m_objDataBuf;

	CBuffer           m_objReqBuf;
	CBuffer           m_objResBuf;

	LPFN_CONNECTEX    m_lpfnConnectEx;
	LPFN_DISCONNECTEX m_lpfnDisconnectEx;

	CRITICAL_SECTION  m_csLock;

	CSocketContext* m_pNext;

public:
	CSocketContext()
	{
		m_hSocket = INVALID_SOCKET;
		m_lToken = 0;
		ZeroMemory(&m_local, sizeof(m_local));
		ZeroMemory(&m_remote, sizeof(m_remote));
		m_bClose = FALSE;
		m_nNextReadSerialNo = 0;
		m_nCurrentReadSerialNo = 0;
		m_nPostedSend = 0;
		m_nPostedRecv = 0;
		m_nPendingRecv = 0;
		m_pWaitingRecv = NULL;
		m_nPendingSend = 0;
		m_pWaitingSend = NULL;
		m_nPacketNo = 0;
		m_nSessionID = 0;
		::InitializeCriticalSection(&m_csLock);
		m_pNext = NULL;
	}
	virtual ~CSocketContext()
	{
		if (m_pWaitingRecv)
		{
			SAFE_DELETE(m_pWaitingRecv);
		}
		if (m_pWaitingSend)
		{
			SAFE_DELETE(m_pWaitingSend);
		}
		if (m_pNext)
		{
			SAFE_DELETE(m_pNext);
		}
		::DeleteCriticalSection(&m_csLock);
	}
};

class CSocketListenContext {
public:
	SOCKET                     m_hSocket;
	HANDLE                     m_hAcceptHandle;
	HANDLE                     m_hRepostHandle;
	CSocketBuffer*             m_pAcceptPendingList;
	UINT                       m_nAcceptPendingListCount;
	UINT                       m_nInitAccpets;
	UINT                       m_nRepostCount;
	LPFN_ACCEPTEX              m_lpfnAcceptEx;
	LPFN_GETACCEPTEXSOCKADDRS  m_lpfnGetAcceptExSockaddrs;
	CRITICAL_SECTION           m_csLock;
	
	CSocketListenContext*      m_pNext;

public:
	CSocketListenContext()
	{
		::InitializeCriticalSection(&m_csLock);
		m_pAcceptPendingList = NULL;
		m_lpfnAcceptEx = NULL;
		m_lpfnGetAcceptExSockaddrs = NULL;
		m_pNext = NULL;
		m_nAcceptPendingListCount = 0;
		m_nInitAccpets = 0;
		m_nRepostCount = 0;
	}
	virtual ~CSocketListenContext()
	{
		if (m_pAcceptPendingList)
		{
			SAFE_DELETE(m_pAcceptPendingList);
		}
		if (m_pNext)
		{
			SAFE_DELETE(m_pNext);
		}
		m_lpfnAcceptEx = NULL;
		m_lpfnGetAcceptExSockaddrs = NULL;
		::DeleteCriticalSection(&m_csLock);
	}
};

class REQUEST;
typedef REQUEST* LPREQUEST;
class REQUEST
{
public:
	REQUEST()
	{
		m_stHead = { 0 };
		m_nDataLen = 0;
		m_pDataPtr = nullptr;
	}
	REQUEST(UINT nRequest, void* pData, int nLen)
	{
		m_stHead.nRequest = nRequest;
		m_nDataLen = nLen;
		m_pDataPtr = pData;
	}
	virtual ~REQUEST()
	{
		m_stHead = { 0 };
		m_nDataLen = 0;
	}
public:
	REQUEST_HEAD m_stHead;
	UINT         m_nDataLen;
	void*        m_pDataPtr;
};

class CIocpServer{
public:
	CIocpServer();
	virtual ~CIocpServer();

	virtual void InitializeMembers();
	virtual bool BeginBindListen(const char* lpSzIp, UINT nPort, UINT nInitAccepts, UINT nMaxAccpets);
	virtual bool BeginThreadPool(UINT nThreads, UINT nConcurrency);
	virtual bool InitializeIo();
	virtual bool Initialize(const char* lpSzIp, UINT nPort, UINT nInitAccepts, UINT nMaxAccpets, UINT nMaxSocketBufferListCount, UINT nMaxSocketContextListCount, UINT nMaxSendCount, UINT nThreads, UINT nConcurrency, UINT nMaxConnections);
	virtual bool Shutdown();

	virtual BOOL OnRequest(void* lpParam1, void* lpParam2);

	virtual bool PostAccept(CSocketContext* pContext, CSocketBuffer* pBuffer, DWORD& dwWSAError);
	virtual bool PostRecv(CSocketContext* pContext, CSocketBuffer* pBuffer, DWORD& dwWSAError);
	virtual bool PostSend(CSocketContext* pContext, CSocketBuffer* pBuffer, DWORD& dwWSAError);

	virtual bool OnReceiveData(CSocketContext* pContext, CSocketBuffer* pBuffer);
	virtual bool OnCheckHeader(void* pData);
	virtual bool OnVerifyData(CSocketContext* pContext, CSocketBuffer* pBuffer);
	virtual BOOL OnHandleData(CSocketContext* pContext, CSocketBuffer* pBuffer);

	virtual bool ConnectTo(SOCKET hSocket, const char* szIp, const int nPort);
	virtual bool SendData(SOCKET hSocket, LPCONTEXT_HEAD lpContextHead, LPREQUEST lpRequest, UINT uiMsgType);

	virtual void ComposePacket(CBuffer& dstBuf, UINT nMsgType, LPVOID pData, UINT nPacketSize);
	virtual void DecomposePacket(CBuffer& srcBuf, CBuffer& dstBuf, int nPacketSize);

	virtual void HandleIo(DWORD dwKey, CSocketBuffer* pBuffer, DWORD dwTrans, DWORD dwError);
	virtual void HandleIoDefault(DWORD dwKey, CSocketBuffer* pBuffer, DWORD dwTrans, DWORD dwError);
	virtual void HandleIoAccept(DWORD dwKey, CSocketBuffer* pBuffer, DWORD dwTrans, DWORD dwError);
	virtual void HandleIoRead(DWORD dwKey, CSocketBuffer* pBuffer, DWORD dwTrans, DWORD dwError);
	virtual void HandleIoWrite(DWORD dwKey, CSocketBuffer* pBuffer, DWORD dwTrans, DWORD dwError);

	virtual BOOL InsertPendingAccepts(CSocketContext* pContext, CSocketBuffer* pBuffer);
	virtual BOOL RemovePendingAccepts(CSocketBuffer* pBuffer);

	virtual BOOL InsertConnectionContext(CSocketContext* pContext);
	virtual BOOL CloseConnectionContext(CSocketContext* pContext);
	virtual BOOL ClearConnectionContext();

	virtual CSocketBuffer* AllocateSocketBuffer(UINT nBufferLen);
	virtual void ReleaseSocketBuffer(CSocketBuffer* pBuffer);
	virtual void FreeSocketBuffer();
	virtual CSocketContext* AllocateSocketContext(SOCKET socket);
	virtual void ReleaseSocketContext(CSocketContext* pContext);
	virtual void FreeSocketContext();

	virtual CSocketContext* FindClient(SOCKET hSocket);

	virtual CSocketBuffer* GetRecvSocketBuffer(CSocketContext* pContext, CSocketBuffer* pBuffer);
	virtual CSocketBuffer* GetSendSocketBuffer(CSocketContext* pContext, CSocketBuffer* pBuffer);

protected:
	static unsigned __stdcall AcceptThreadFunc(LPVOID lpParam);
	static unsigned __stdcall WorkerThreadFunc(LPVOID lpParam);

private:
	void* _GetExtendFunc(const SOCKET& socket, const GUID& guid);

public:
	/*空闲I/O操作结构列表*/
	CCritSec m_csSocketBufferList;
	CSocketBuffer* m_pSocketBufferList;
	UINT m_nSocketBufferListCount;
	UINT m_nMaxSocketBufferListCount;

	/*空闲套接字上下文列表*/
	CCritSec m_csSocketContextList;
	CSocketContext* m_pSocketContextList;
	UINT m_nSocketContextListCount;
	UINT m_nMaxSocketContextListCount;

	/*连接套接字上下文列表*/
	CCritSec m_csConnectionContext;
	CSocketContext* m_pConnectionContext;
	UINT m_nConnectionContextCount;
	UINT m_nMaxConnections;

	/*监听套接字上下文*/
	CSocketListenContext* m_pListenContext;
	UINT m_nMaxAccepts;

	/*服务IP/端口*/
	TCHAR m_szIp[IP_LENGTH];
	UINT m_nPort;

	/*完成端口句柄*/
	HANDLE m_hCompletionPort;

	/*监听线程*/
	HANDLE m_hAcceptThread;
	UINT m_uiAcceptThread;

	/*事件句柄*/
	HANDLE m_hWaitHandle[2];
	HANDLE m_hWorkerHandle[MAX_WORKER_THREAD_NUMBER];

	UINT m_nMaxSendCount;
	UINT m_nWorkerThreads;
	UINT m_nConcurrency;
	
	BOOL m_bShutdown;
};

class CIocpClient : public CIocpServer{
public:
	CIocpClient();
	virtual ~CIocpClient();

public:
	virtual BOOL Create(const char* lpSzIp, UINT nPort, UINT nMaxConnections, UINT nThreads, UINT nConcurrency);
	virtual BOOL BeginConnect(const char* lpSzIp, UINT nPort);
	virtual BOOL Destroy();

	virtual bool OnReceiveData(CSocketContext* pContext, CSocketBuffer* pBuffer);

	virtual BOOL SendData(SOCKET hSocket, LPCONTEXT_HEAD lpContextHead, LPREQUEST lpRequest, UINT uiMsgType);
	virtual BOOL SendCast(SOCKET hSocket, LPCONTEXT_HEAD lpContextHead, LPREQUEST lpRequest, UINT uiMsgType);
};