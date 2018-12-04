#include "stdafx.h"

CBaseServer::CBaseServer()
:m_csVectClientContext()
{
	m_szIp = "127.0.0.1";
	//GetPrivateProfileString(_T("Server"), _T("ip"), _T("127.0.0.1"), m_szIp, sizeof(m_szIp), _getIniFile());
	m_nPort = 8888;
	m_IocpModule = new CIOCPModule(this);
	m_IocpAccept = new CIOCPAccept();
	m_IocpSocket = new CIOCPSocket();
	m_hShutdownEvent = NULL;
	CAutoLock lock(&m_csVectClientContext);
	m_vectClientConetxt.clear();
	m_pListenContext = NULL;
	m_pFnAcceptEx = NULL;
	m_pFnGetAcceptExSockAddr = NULL;

	OnWorkerStart(m_IocpModule);

	m_nTick = 0;
	HANDLE handle = (HANDLE)_beginthreadex(NULL, 0, _heartbeatFunc, this, 0, NULL);
}

CBaseServer::~CBaseServer()
{
	OnWorkerExit();
}

BOOL CBaseServer::OnWorkerStart(CIOCPModule* pCIocModule)
{
	pCIocModule->m_hShutdownEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hShutdownEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	return TRUE;
}
void CBaseServer::OnWorkerExit()
{
	SAFE_RELEASE_HANDLE(m_hShutdownEvent);

	for (auto i = 0; i < m_IocpModule->m_nWorkerThreads; i++)
	{
		SAFE_RELEASE_HANDLE(m_IocpModule->m_pWorkerThread[i]);
	}
	SAFE_DELETE(m_IocpModule->m_pWorkerThread);
	SAFE_RELEASE_HANDLE(m_IocpModule->m_hIocp);
	SAFE_DELETE(m_pListenContext);

	//LOG_INFO("The Class Members had All Released...");
}

BOOL CBaseServer::Initialize()
{
	//CAutoLock lock(&m_csVectClientContext);
	m_csVectClientContext.Lock();

	if (FALSE == LoadSocketLib())
	{
		//LOG_ERROR("CBaseServer::LoadSocketLib Initialize Failed...");
	}
	//LOG_INFO("CBaseServer::LoadSocketLib Initialize Successful...");

	if (FALSE == m_IocpModule->Initialize())
	{
		//LOG_ERROR("CIOCPModule Initialize Failed...");
	}
	//LOG_INFO("CBaseServer::LoadSocketLib Initialize Successful...");

	//AcceptEx和GetAcceptExSockAddr的GUID，用于导出函数指针
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	GUID GuidGetAcceptExSockAddr = WSAID_GETACCEPTEXSOCKADDRS;

	sockaddr_in socketAddr;

	m_pListenContext = new SOCKET_CONTEXT;
	m_pListenContext->m_Socket = INVALID_SOCKET;
	memset(&m_pListenContext->m_SockAddrIn, 0, sizeof(m_pListenContext->m_SockAddrIn));

	m_pListenContext->m_Socket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == m_pListenContext->m_Socket)
	{
		//LOG_ERROR("CBaseServer::Initialize Initialize the WSASocket Failed...");
		return FALSE;
	}
	//LOG_INFO("CBaseServer::Initialize Initialize the WSASocket Successful...");

	if (NULL == ::CreateIoCompletionPort((HANDLE)m_pListenContext->m_Socket, (HANDLE)m_IocpModule->m_hIocp, (DWORD)m_pListenContext, 0))
	{
		//LOG_ERROR("Bind the ClientContext to IoCompletionPort Failed...");
		SAFE_RELEASE_SOCKET(m_pListenContext->m_Socket);
		return FALSE;
	}
	//LOG_INFO("Bind the ClientContext to IoCompletionPort Successful...");

	memset(&socketAddr, 0, sizeof(sockaddr_in));
	socketAddr.sin_family = AF_INET;
	socketAddr.sin_addr.S_un.S_addr = inet_addr(m_szIp);
	socketAddr.sin_port = htons((u_short)m_nPort);

	auto nRet = ::bind(m_pListenContext->m_Socket, (struct sockaddr*)&socketAddr, sizeof(socketAddr));
	if (SOCKET_ERROR == nRet)
	{
		//LOG_ERROR("Bind SOCKET Failed...");
		return FALSE;
	}
	//LOG_INFO("Bind SOCKET Successful...");

	nRet = ::listen(m_pListenContext->m_Socket, SOMAXCONN);
	if (SOCKET_ERROR == nRet)
	{
		//LOG_ERROR("Listen SOCKET Failed...");
		return FALSE;
	}
	//LOG_INFO("Listen SOCKET Successful...");

	DWORD dwBytes = 0;
	//使用AcceptEx函数，获取函数指针
	if (SOCKET_ERROR == ::WSAIoctl(
		m_pListenContext->m_Socket,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidAcceptEx,
		sizeof(GuidAcceptEx),
		&m_pFnAcceptEx,
		sizeof(m_pFnAcceptEx),
		&dwBytes,
		NULL,
		NULL))
	{
		//LOG_ERROR("WSAIoctl do not get the AcceptEx Pointer...");
		return FALSE;
	}

	//使用GetAcceptExSockAddr函数，获取函数指针
	if (SOCKET_ERROR == ::WSAIoctl(
		m_pListenContext->m_Socket,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidGetAcceptExSockAddr,
		sizeof(GuidGetAcceptExSockAddr),
		&m_pFnGetAcceptExSockAddr,
		sizeof(m_pFnGetAcceptExSockAddr),
		&dwBytes,
		NULL,
		NULL))
	{
		//LOG_ERROR("WSAIoctl do not get the GetAcceptExSockAddr Pointer...");
		return FALSE;
	}

	//为AcceptEx准备参数，然后投递AcceptEx IO请求
	for (auto i = 0; i < MAX_MEANWHILE_POST_ACCEPT; i++)
	{
		LPIO_CONTEXT pAcceptIoContext = new IO_CONTEXT;
		ZeroMemory(&pAcceptIoContext->m_Overlapped, sizeof(OVERLAPPED));
		ZeroMemory(&pAcceptIoContext->m_szBuffer, DATA_BUF_SIZE);
		pAcceptIoContext->m_WsaBuf.buf = pAcceptIoContext->m_szBuffer;
		pAcceptIoContext->m_WsaBuf.len = DATA_BUF_SIZE;
		pAcceptIoContext->m_Socket = INVALID_SOCKET;
		pAcceptIoContext->m_OpeType = OPE_NULL;

		m_pListenContext->m_vectIoContext.push_back(pAcceptIoContext);

		if (FALSE == m_IocpAccept->PostAccept(pAcceptIoContext, this))
		{
			std::vector<LPIO_CONTEXT>::iterator iter;
			for (iter = m_pListenContext->m_vectIoContext.begin(); iter != m_pListenContext->m_vectIoContext.end();)
			{
				if (pAcceptIoContext == *iter)
				{
					delete pAcceptIoContext;
					pAcceptIoContext = NULL;
					iter = m_pListenContext->m_vectIoContext.erase(iter);
				}
				else
				{
					++iter;
				}
			}

			return FALSE;
		}
		//LOG_INFO("Posted AcceptEx Request...");
	}

	m_csVectClientContext.Unlock();

	return TRUE;
}
void CBaseServer::Shutdown()
{
	if (NULL != m_pListenContext && INVALID_SOCKET == m_pListenContext->m_Socket)
	{
		SetEvent(m_hShutdownEvent);
		for (auto i = 0; i < m_IocpModule->m_nWorkerThreads; i++)
		{
			PostQueuedCompletionStatus(m_IocpModule->m_hIocp, 0, (DWORD)NULL, NULL);
		}
		WaitForMultipleObjects(m_IocpModule->m_nWorkerThreads, m_IocpModule->m_pWorkerThread, TRUE, INFINITE);

		CAutoLock lock(&m_csVectClientContext);
		std::vector<LPSOCKET_CONTEXT>().swap(m_vectClientConetxt);

		this->OnWorkerExit();

		//LOG_INFO("The Server has Shutdown...Stopped Listening...");
	}
}

BOOL CBaseServer::LoadSocketLib()
{
	WSADATA wsaData;
	auto nRet = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (NO_ERROR != nRet)
	{
		//LOG_ERROR("Initialized the Socket Libs Failed...");
		return FALSE;
	}
	//LOG_INFO("Initialized the Socket Libs Successful...");

	return TRUE;
}
void CBaseServer::UnloadSocketLib()
{
	WSACleanup();
}

void CBaseServer::TestSend()
{
	LPIO_CONTEXT pIoContext = new IO_CONTEXT;
	ZeroMemory(&pIoContext->m_Overlapped, sizeof(OVERLAPPED));
	ZeroMemory(&pIoContext->m_szBuffer, DATA_BUF_SIZE);
	pIoContext->m_WsaBuf.len = DATA_BUF_SIZE;
	pIoContext->m_Socket = INVALID_SOCKET;

	CAutoLock lock(&m_csVectClientContext);
	pIoContext->m_OpeType = OPE_SEND;
	for (unsigned i = 0; i < m_vectClientConetxt.size(); i++)
	{
		pIoContext->m_Socket = m_vectClientConetxt.at(i)->m_Socket;
		strcpy(pIoContext->m_szBuffer, "TestSend");
		pIoContext->m_WsaBuf.buf = pIoContext->m_szBuffer;
#if 1
		
#else
		
#endif

		m_IocpSocket->DoSend(m_vectClientConetxt.at(i), pIoContext, this);
	}
}

unsigned int CBaseServer::_heartbeatFunc(LPVOID pParam)
{
	CBaseServer* pServer = (CBaseServer*)pParam;

	while (!pServer->m_hShutdownEvent)
	{
		//pServer->m_nTick = _getSencond();
		unsigned int uiSlot = pServer->m_nTick % HEART_BEAT_WHEEL_SLOT;
		std::vector<SOCKET> list = pServer->m_listHeartBeatWheel[uiSlot];
		pServer->m_listHeartBeatWheel[uiSlot].clear();
		for (auto iter = list.begin(); iter != list.end(); iter++)
		{
			HEART_BEAT_DETECT stuHeartBeatDetect = pServer->m_mapClientHeartBeat.at(*iter);
			auto clientSocket = pServer->m_mapClientSocket.find(*iter);
			auto nUserID = clientSocket->first;
			pServer->_detectHeartBeat(nUserID, uiSlot, stuHeartBeatDetect);
		}
		Sleep(TIME_TICK - 1);
		pServer->m_nTick++;
	}

	return 0;
}

void CBaseServer::_detectHeartBeat(unsigned int nUserID, unsigned int& nIndex, HEART_BEAT_DETECT& stuHeartBeatDetect)
{
	SOCKET socket = stuHeartBeatDetect.m_Socket;
	unsigned int uiCount = stuHeartBeatDetect.m_uiDisconnectCount;
	//心跳缺失检测三次以上直接断开该Socket的连接
	if (uiCount >= 3)
	{
		::closesocket(socket);

		{
			CAutoLock lock(&this->m_csMapClientHeartBeat);
			this->m_mapClientHeartBeat.erase(socket); 
		}
		{
			CAutoLock lock(&m_csMapClientHeartBeat);
			this->m_mapClientHeartBeat.erase(nUserID);
		}
		
	}
	//心跳检测正常的重新放入循环
	else
	{
		if (0 == nIndex)
		{
			CAutoLock lock(&this->m_csHeartBeatWheel);
			this->m_listHeartBeatWheel[HEART_BEAT_WHEEL_SLOT - 1].push_back(socket);
		}
		else
		{
			CAutoLock lock(&this->m_csHeartBeatWheel);
			this->m_listHeartBeatWheel[nIndex - 1].push_back(socket);
		}
	}
}

TCHAR* CBaseServer::_getIniFile()
{
	TCHAR szIniFile[MAX_PATH] = { 0 };
	TCHAR szFullName[MAX_PATH] = { 0 };
	GetModuleFileName(GetModuleHandle(NULL), szFullName, sizeof(szFullName));
	const char szTemp[MAX_PATH] = { 0 };
	WideCharToMultiByte(CP_UTF8, 0, szFullName, MAX_PATH, (LPSTR)szTemp, MAX_PATH, 0, 0);
	const char* ptr = strrchr(szTemp, '\\');

	string strStr = szTemp;
	auto nIndex = strStr.find(ptr);
	string strRes = strStr.substr(0, nIndex);
	strRes.append("\\CPlusServer.ini");

	StringToTchar(strRes, szIniFile);

	return szIniFile;
}