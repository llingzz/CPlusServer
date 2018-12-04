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

private:
	static unsigned int __stdcall _heartbeatFunc(LPVOID pParam);
	inline void _detectHeartBeat(unsigned int nUserID, unsigned int& nIndex, HEART_BEAT_DETECT& stuHeartBeatDetect);
public:
	TCHAR* _getIniFile();

public:
	CHAR*							m_szIp;						//服务器IP
	int								m_nPort;					//服务器端口

	CIOCPModule*					m_IocpModule;				//IOCP对象
	CIOCPAccept*					m_IocpAccept;				//IOCPAccept对象
	CIOCPSocket*					m_IocpSocket;				//IOCPSocket对象

	HANDLE							m_hShutdownEvent;			//服务器关闭句柄

	CritSec							m_csVectClientContext;		//vector的临界区对象
	std::vector<LPSOCKET_CONTEXT>	m_vectClientConetxt;		//存储当前所有连接客户端的Context信息

	CritSec							m_csMapSocket;				//连接客户端Map表的临界区对象
	std::map<unsigned int, SOCKET>	m_mapClientSocket;			//存储当前所有连接客户端的Socket信息

	CritSec							m_csMapClientHeartBeat;		//连接客户端Map表的临界区对象
	std::map<SOCKET, HEART_BEAT_DETECT> m_mapClientHeartBeat;		//存储当前所有连接客户端的心跳检测信息

	unsigned long					m_nTick;					//读秒计数
	CritSec							m_csHeartBeatWheel;			//心跳检测轮临界区对象
	std::vector<SOCKET>				m_listHeartBeatWheel[HEART_BEAT_WHEEL_SLOT];
																//心跳检测轮

	LPSOCKET_CONTEXT				m_pListenContext;			//监听客户端的Context信息

	LPFN_ACCEPTEX					m_pFnAcceptEx;				//AcceptEx的函数指针
	LPFN_GETACCEPTEXSOCKADDRS		m_pFnGetAcceptExSockAddr;	//GetAcceptExSockAddr的函数指针
};