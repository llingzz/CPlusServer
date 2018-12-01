#pragma once

class CBaseServer{
public:
	CBaseServer();
	~CBaseServer();

public:
	BOOL Initialize();
	void Shutdown();

	BOOL OnWorkerStart(CIOCPModule* pCIocModule);
	void OnWorkerExit();

	BOOL LoadSocketLib();
	void UnloadSocketLib();

	void TestSend();

public:
	CHAR*							m_szIp;						//服务器IP
	int								m_nPort;					//服务器端口

	CIOCPModule*					m_IocpModule;				//IOCP对象
	CIOCPAccept*					m_IocpAccept;				//IOCPAccept对象
	CIOCPSocket*					m_IocpSocket;				//IOCPSocket对象

	HANDLE							m_hShutdownEvent;			//服务器关闭句柄

	CritSec							m_csVectClientContext;		//vector的临界区对象
	std::vector<LPSOCKET_CONTEXT>	m_vectClientConetxt;		//存储当前所有连接客户端的Context信息

	LPSOCKET_CONTEXT				m_pListenContext;			//监听客户端的Context信息

	LPFN_ACCEPTEX					m_pFnAcceptEx;				//AcceptEx的函数指针
	LPFN_GETACCEPTEXSOCKADDRS		m_pFnGetAcceptExSockAddr;	//GetAcceptExSockAddr的函数指针
};