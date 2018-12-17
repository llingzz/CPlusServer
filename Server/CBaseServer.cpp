#include "stdafx.h"

CBaseServer::CBaseServer()
:m_csVectClientContext()
{
	GetPrivateProfileString(_T("Server"), _T("ip"), _T(""), m_szIp, IP_LENGTH, GetIniFileName());
	m_nPort = GetPrivateProfileInt(_T("Server"), _T("port"), 0, GetIniFileName());
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

	printf("The Class Members had All Released...\n");
}

BOOL CBaseServer::Initialize()
{
	unsigned int uiThreadID;
	HANDLE handle = (HANDLE)_beginthreadex(NULL, 0, heartbeatFunc, this, 0, &uiThreadID);
	CAutoLock lock(&m_csVectClientContext);
	if (FALSE == LoadSocketLib())
	{
		printf("CBaseServer::LoadSocketLib Initialize Failed...\n");
	}
	printf("CBaseServer::LoadSocketLib Initialize Successful...\n");

	if (FALSE == m_IocpModule->Initialize())
	{
		printf("CIOCPModule Initialize Failed...\n");
	}
	printf("CBaseServer::LoadSocketLib Initialize Successful...\n");

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
		printf("CBaseServer::Initialize Initialize the WSASocket Failed...\n");
		return FALSE;
	}
	printf("CBaseServer::Initialize Initialize the WSASocket Successful...\n");

	if (NULL == ::CreateIoCompletionPort((HANDLE)m_pListenContext->m_Socket, (HANDLE)m_IocpModule->m_hIocp, (DWORD)m_pListenContext, 0))
	{
		printf("Bind the ClientContext to IoCompletionPort Failed...\n");
		SAFE_RELEASE_SOCKET(m_pListenContext->m_Socket);
		return FALSE;
	}
	printf("Bind the ClientContext to IoCompletionPort Successful...\n");

	memset(&socketAddr, 0, sizeof(sockaddr_in));
	socketAddr.sin_family = AF_INET;
	char szTemp[IP_LENGTH] = { 0 };
	TcharToChar(m_szIp, szTemp);
	socketAddr.sin_addr.S_un.S_addr = inet_addr(szTemp);
	socketAddr.sin_port = htons((u_short)m_nPort);

	auto nRet = ::bind(m_pListenContext->m_Socket, (struct sockaddr*)&socketAddr, sizeof(socketAddr));
	if (SOCKET_ERROR == nRet)
	{
		printf("Bind SOCKET Failed...\n");
		return FALSE;
	}
	printf("Bind SOCKET Successful...\n");

	nRet = ::listen(m_pListenContext->m_Socket, SOMAXCONN);
	if (SOCKET_ERROR == nRet)
	{
		printf("Listen SOCKET Failed...\n");
		return FALSE;
	}
	printf("Listen SOCKET Successful...\n");

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
		printf("WSAIoctl do not get the AcceptEx Pointer...\n");
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
		printf("WSAIoctl do not get the GetAcceptExSockAddr Pointer...\n");
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
		printf("Posted AcceptEx Request...\n");
	}

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

		printf("The Server has Shutdown...Stopped Listening...\n");
	}
}

BOOL CBaseServer::LoadSocketLib()
{
	WSADATA wsaData;
	auto nRet = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (NO_ERROR != nRet)
	{
		printf("Initialized the Socket Libs Failed...\n");
		return FALSE;
	}
	printf("Initialized the Socket Libs Successful...\n");

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

unsigned int CBaseServer::heartbeatFunc(LPVOID pParam)
{
	CBaseServer* pServer = (CBaseServer*)pParam;

	while (1)
	{
		if (0 == (pServer->m_nTick % 10))
		{
			printf("Ticks:%d Detect Pluse...Current Connections:%d\n", pServer->m_nTick, pServer->getConnections());
		}
		unsigned int uiSlot = pServer->m_nTick % HEART_BEAT_WHEEL_SLOT;
		CAutoLock lock(&pServer->m_csHeartBeatWheel);
		std::vector<SOCKET> list = pServer->m_vectHeartBeatWheel[uiSlot];
		pServer->m_vectHeartBeatWheel[uiSlot].clear();
		for (auto iter = list.begin(); iter != list.end(); iter++)
		{
			CAutoLock lock(&pServer->m_csMapClientHeartBeat);
			std::map<SOCKET, HEART_BEAT_DETECT>::iterator iterTemp = pServer->m_mapClientHeartBeat.find(*iter);
			if (iterTemp != pServer->m_mapClientHeartBeat.end())
			{
				pServer->detectHeartBeat(uiSlot, pServer->m_mapClientHeartBeat.at(*iter));
			}
		}
		Sleep(TIME_TICK);
		pServer->m_nTick++;
	}

	return 0;
}

void CBaseServer::detectHeartBeat(unsigned int& nIndex, HEART_BEAT_DETECT& stuHeartBeatDetect)
{
	SOCKET socket = stuHeartBeatDetect.m_Socket;
	if ((m_nTick - stuHeartBeatDetect.m_uiLastTick) >= HEART_BEAT_WHEEL_SLOT)
	{
		stuHeartBeatDetect.m_uiDisconnectCount++;
	}
	//心跳缺失检测三次以上直接断开该Socket的连接
	if (stuHeartBeatDetect.m_uiDisconnectCount > MAX_DISCONNECTION_TIMES)
	{
		ClientClosed(socket);
	}
	//心跳检测正常的重新放入循环
	else
	{
		if (0 == nIndex)
		{
			CAutoLock lock(&this->m_csHeartBeatWheel);
			this->m_vectHeartBeatWheel[HEART_BEAT_WHEEL_SLOT - 1].push_back(socket);
		}
		else
		{
			CAutoLock lock(&this->m_csHeartBeatWheel);
			this->m_vectHeartBeatWheel[nIndex - 1].push_back(socket);
		}
	}
}

BOOL CBaseServer::SendRequest(SOCKET client)
{
	return TRUE;
}
BOOL CBaseServer::SendResponse(SOCKET client)
{
	return TRUE;
}
void CBaseServer::OnRequest(void* pParam1, void* pParam2)
{
	LPMESSAGE_HEAD lpMessageHead = (LPMESSAGE_HEAD)pParam1;
	LPMESSAGE_CONTENT lpMessageContent = (LPMESSAGE_CONTENT)pParam2;

	switch (lpMessageContent->nRequest)
	{
	case PROTOCOL_HEART_PLUSE:
		printf("Client Send Pluse Request...\n");
		OnHeartPluse(lpMessageHead, lpMessageContent);
		break;
	default:
		break;
	}
}
void CBaseServer::OnResponse(void* pParam1, void* pParam2)
{

}

BOOL CBaseServer::OnHeartPluse(LPMESSAGE_HEAD pMessageHead, LPMESSAGE_CONTENT pMessageContent)
{
	CAutoLock lock(&m_csMapClientHeartBeat);
	std::map<SOCKET, HEART_BEAT_DETECT>::iterator iter = m_mapClientHeartBeat.find(pMessageHead->hSocket);
	if (iter != m_mapClientHeartBeat.end())
	{
		m_mapClientHeartBeat[pMessageHead->hSocket].m_uiDisconnectCount = 0;
		m_mapClientHeartBeat[pMessageHead->hSocket].m_uiLastTick = m_nTick;
	}
	else
	{
		HEART_BEAT_DETECT stuHeartBeatDetect = { 0 };
		stuHeartBeatDetect.m_Socket = pMessageHead->hSocket;
		stuHeartBeatDetect.m_uiDisconnectCount = 0;
		stuHeartBeatDetect.m_uiLastTick = m_nTick;
		m_mapClientHeartBeat.insert(std::map<SOCKET, HEART_BEAT_DETECT>::value_type(stuHeartBeatDetect.m_Socket, stuHeartBeatDetect));
	}

	return TRUE;
}

void CBaseServer::ClientClosed(SOCKET scoSocket)
{
	{
		CAutoLock lock(&m_csMapClientHeartBeat);
		std::map<SOCKET, HEART_BEAT_DETECT>::iterator iter = m_mapClientHeartBeat.find(scoSocket);
		if (iter != m_mapClientHeartBeat.end())
		{
			m_mapClientHeartBeat.erase(iter);
		}
	}
	{
		CAutoLock lock(&m_csHeartBeatWheel);
		for (auto i = 0; i < HEART_BEAT_WHEEL_SLOT; i++)
		{
			for (auto iter = m_vectHeartBeatWheel[i].begin(); iter != m_vectHeartBeatWheel[i].end();)
			{
				if (scoSocket == *iter)
				{
					iter = m_vectHeartBeatWheel[i].erase(iter);
					break;
				}
				else
				{
					++iter;
				}
			}
		}
	}
	::closesocket(scoSocket);
}

LPCTSTR CBaseServer::GetIniFileName()
{
	getIniFile();
	return m_szIniFilePath;
}

void CBaseServer::getIniFile()
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
	//StringReplaceAllSubs(strRes, "\\", "\\\\");
	StringToTchar(strRes, szIniFile);
	//std::wcout << szIniFile << std::endl;
	//_tprintf(szIniFile);
	lstrcpy(m_szIniFilePath, szIniFile);
}
unsigned int CBaseServer::getConnections()
{
	CAutoLock lock(&m_csHeartBeatWheel);
	unsigned int uiCount = 0;
	for (auto i = 0; i < HEART_BEAT_WHEEL_SLOT; i++)
	{
		uiCount += m_vectHeartBeatWheel[i].size();
	}

	return uiCount;
}