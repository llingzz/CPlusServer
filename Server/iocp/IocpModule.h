#pragma once

#include <winsock2.h>
#include <MSWSock.h>
#pragma comment(lib,"ws2_32.lib")

#include "../common/CommonDef.h"

class CBaseServer;

class CIOCPModule{
public:
	CIOCPModule(CBaseServer* pServer);
	~CIOCPModule();

public:
	BOOL Initialize();
	void DeInitialize();

	static unsigned int __stdcall WorkerThreadFunc(LPVOID lpParam);

	void RemoveSocketContext(LPSOCKET_CONTEXT pSocketContext);

	BOOL HandleErrors(LPSOCKET_CONTEXT pSocketContext, const DWORD& dwErr);

public:
	CBaseServer*					m_pServer;

	HANDLE							m_hIocp;				//完成端口句柄
	HANDLE*							m_pWorkerThread;		//工作线程指针
	HANDLE							m_hShutdownEvent;		//线程退出标志事件

	int								m_nWorkerThreads;		//工作线程数
};