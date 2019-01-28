#pragma once

typedef std::vector<LPSOCKET_CONTEXT> CSocketContextVector;
typedef std::vector<SOCKET> CSocketVector;
typedef std::map<SOCKET, PLUSE_PACKAGE> CClientPluseMap;

class CPlusServer{
public:
	CPlusServer();
	~CPlusServer();

public:
	BOOL Initialize();
	BOOL Shutdown();

	BOOL InitializeIocp();

public:
	BOOL OnClientPluse(LPMESSAGE_HEAD lpMessageHead, LPMESSAGE_CONTENT lpMessageContent);

private:
	BOOL PostAccept(LPIO_CONTEXT pIoContext);
	BOOL DoAccept(LPSOCKET_CONTEXT pSocketContext, LPIO_CONTEXT pIoContext);
	BOOL PostSend(LPIO_CONTEXT pIoContext);
	BOOL DoSend(LPSOCKET_CONTEXT pSocketContext, LPIO_CONTEXT pIoContext);
	BOOL PostRecv(LPIO_CONTEXT pIoContext);
	BOOL DoRecv(LPIO_CONTEXT pIoContext);

	BOOL SendRequest(SOCKET sClient, void* pDataPtr, int nDataLen);
	BOOL SendResponse(SOCKET sClient, void* pDataPtr, int nDataLen);
	BOOL OnRequest(void* pParam1, void* pParam2);
	BOOL OnResponse(void* pParam1, void* pParam2);
	
	BOOL HandleErrors(LPSOCKET_CONTEXT pSocketContext, const DWORD& dwErr);
	BOOL HandleMessage(LPIO_CONTEXT pIoContext, int nOpeType);
	void CreateThreads();
	void CloseClients(SOCKET scoSocket);
	void RemoveSocketContext(LPSOCKET_CONTEXT pSocketContext);
	void SerializeNetMessage(CBufferEx& dataBuffer, LPMESSAGE_HEAD lpMessageHead, LPMESSAGE_CONTENT lpMessageContent, void* pData, int nLen);
	void DeserializeNetMessage(LPIO_CONTEXT pIoContext, LPMESSAGE_HEAD pMessageHead, LPMESSAGE_CONTENT pMessageContent);

	inline BOOL FoundClientSocket(SOCKET sSocket);
	inline BOOL FindClientPluse(SOCKET sSocket, PLUSE_PACKAGE& rPlusePackage);

	LPCTSTR GetIniFilePath();
	static unsigned __stdcall WorkerThreadFunc(LPVOID lpParam);
	static unsigned __stdcall DealerThreadFunc(LPVOID lpParam);
	static unsigned __stdcall ClientPluseFunc(LPVOID lpParam);

public:
	TCHAR								m_szIp[IP_LENGTH];							//服务器IP
	int									m_nPort;									//服务器端口
	unsigned long						m_nTick;									//读秒计数
	TCHAR								m_szIniFilePath[MAX_PATH];					//服务器ini配置文件路径

	HANDLE								m_hIocp;									//完成端口句柄
	HANDLE								m_hShutdownEvent;							//服务器关闭句柄

	int									m_nWorkerThreads;							//工作线程数
	HANDLE*								m_phWorkerThread;							//工作线程句柄指针

	HANDLE								m_hClientPluseHandle;						//心跳检测线程句柄
	unsigned int						m_uiClientPluseThreadId;					//心跳检测线程ID

	HANDLE								m_hMessageDealerEvent;						//消息处理事件句柄
	HANDLE								m_hMessgaeDealerHandle;						//消息处理线程句柄
	unsigned int						m_uiMessageDealerThreadId;					//消息处理线程ID

	CCritSec							m_csVectClientContext;						//vector的临界区对象
	CSocketContextVector				m_vectClientConetxt;						//存储当前所有连接客户端的Context信息

	CCritSec							m_csMapClientPluse;							//心跳map的临界区对象
	CClientPluseMap						m_mapClientPluse;							//心跳map

	LPSOCKET_CONTEXT					m_pListenContext;							//监听客户端的Context信息

	LPFN_ACCEPTEX						m_pFnAcceptEx;								//AcceptEx的函数指针
	LPFN_GETACCEPTEXSOCKADDRS			m_pFnGetAcceptExSockAddr;					//GetAcceptExSockAddr的函数指针
};