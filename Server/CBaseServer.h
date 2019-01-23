#pragma once

class CBaseServer{
public:
	CBaseServer();
	~CBaseServer();

public:
	BOOL Initialize();
	void Shutdown();

	BOOL OnWorkerStart(CIOCPModule* pCIocpModule);
	void OnWorkerExit();

	BOOL LoadSocketLib();
	void UnloadSocketLib();

	BOOL CreateMessageDealerThread();

	BOOL SendRequest(SOCKET client, LPMESSAGE_HEAD lpMessageHead, LPMESSAGE_CONTENT lpMessageContent);
	BOOL SendResponse(SOCKET client, LPMESSAGE_HEAD lpMessageHead, LPMESSAGE_CONTENT lpMessageContent);

	void OnRequest(void* pParam1, void* pParam2);
	void OnResponse(void* pParam1, void* pParam2);

	BOOL OnHeartPluse(LPMESSAGE_HEAD pMessageHead, LPMESSAGE_CONTENT pMessageContent);

	void CloseClients(SOCKET scoSocket);

	LPCTSTR GetIniFileName();
private:
	static unsigned int __stdcall heartbeatFunc(LPVOID pParam);
	static unsigned int __stdcall messageDealerFunc(LPVOID pParam);
	inline void detectHeartBeat(unsigned int& nIndex, HEART_BEAT_DETECT& stuHeartBeatDetect);
	void getIniFile();
	unsigned int getConnections();

	LPSOCKET_CONTEXT getClientSocketContext(SOCKET client);

public:
	TCHAR							m_szIp[IP_LENGTH];							//服务器IP
	int								m_nPort;									//服务器端口
	unsigned long					m_nTick;									//读秒计数
	TCHAR							m_szIniFilePath[MAX_PATH];					//服务器ini配置文件路径

	CIOCPModule*					m_IocpModule;								//IOCP对象
	CIOCPAccept*					m_IocpAccept;								//IOCPAccept对象
	CIOCPSocket*					m_IocpSocket;								//IOCPSocket对象

	HANDLE							m_hShutdownEvent;							//服务器关闭句柄
	HANDLE							m_hMessageDealerEvent;						//消息处理事件句柄
	HANDLE							m_hMessgaeDealerHandle;						//消息处理线程句柄
	unsigned int					m_uiMessageDealerThreadId;					//消息处理线程ID

	CCritSec						m_csVectClientContext;						//vector的临界区对象
	std::vector<LPSOCKET_CONTEXT>	m_vectClientConetxt;						//存储当前所有连接客户端的Context信息
	CCritSec							m_csMapClientHeartBeat;					//连接客户端Map表的临界区对象
	std::map<SOCKET, HEART_BEAT_DETECT> m_mapClientHeartBeat;					//存储当前所有连接客户端的心跳检测信息
	CCritSec						m_csHeartBeatWheel;							//心跳检测轮临界区对象
	std::vector<SOCKET>				m_vectHeartBeatWheel[HEART_BEAT_WHEEL_SLOT];//心跳检测轮

	LPSOCKET_CONTEXT				m_pListenContext;							//监听客户端的Context信息

	LPFN_ACCEPTEX					m_pFnAcceptEx;								//AcceptEx的函数指针
	LPFN_GETACCEPTEXSOCKADDRS		m_pFnGetAcceptExSockAddr;					//GetAcceptExSockAddr的函数指针
};