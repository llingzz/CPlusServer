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

BOOL CBaseServer::OnWorkerStart(CIOCPModule* pCIocpModule)
{
	::CoInitialize(NULL);
	pCIocpModule->m_hShutdownEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hShutdownEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	/*CreateMessageDealerThread();
	::WaitForSingleObject(m_hMessageDealerEvent, INFINITE);
	CloseHandle(m_hMessageDealerEvent);*/

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

	::CoUninitialize();
	FILE_INFOS("the class members had all released...");
}

BOOL CBaseServer::Initialize()
{
	unsigned int uiThreadID;
	HANDLE handle = (HANDLE)_beginthreadex(NULL, 0, heartbeatFunc, this, 0, &uiThreadID);
	CAutoLock lock(&m_csVectClientContext);
	if (FALSE == LoadSocketLib())
	{
		return FALSE;
	}

	if (FALSE == m_IocpModule->Initialize())
	{
		return FALSE;
	}

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
		FILE_ERROR("%s initialize the WSASocket failed...", __FUNCTION__);
		return FALSE;
	}

	if (NULL == ::CreateIoCompletionPort((HANDLE)m_pListenContext->m_Socket, (HANDLE)m_IocpModule->m_hIocp, (DWORD)m_pListenContext, 0))
	{
		FILE_ERROR("%s bind the clientcontext to iocompletionport failed...", __FUNCTION__);
		SAFE_RELEASE_SOCKET(m_pListenContext->m_Socket);
		return FALSE;
	}

	memset(&socketAddr, 0, sizeof(sockaddr_in));
	socketAddr.sin_family = AF_INET;
	char szTemp[IP_LENGTH] = { 0 };
	TcharToChar(m_szIp, szTemp);
	socketAddr.sin_addr.S_un.S_addr = inet_addr(szTemp);
	socketAddr.sin_port = htons((u_short)m_nPort);

	auto nRet = ::bind(m_pListenContext->m_Socket, (struct sockaddr*)&socketAddr, sizeof(socketAddr));
	if (SOCKET_ERROR == nRet)
	{
		FILE_ERROR("bind socket failed...");
		return FALSE;
	}

	nRet = ::listen(m_pListenContext->m_Socket, SOMAXCONN);
	if (SOCKET_ERROR == nRet)
	{
		FILE_ERROR("listen socket failed...");
		return FALSE;
	}

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
		FILE_ERROR("WSAIoctl do not get the AcceptEx pointer...");
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
		FILE_ERROR("WSAIoctl do not get the GetAcceptExSockAddr pointer...");
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
		FILE_INFOS("posted AcceptEx request...");
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

		FILE_INFOS("server shutdown!!!");
	}
}

BOOL CBaseServer::LoadSocketLib()
{
	WSADATA wsaData;
	auto nRet = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (NO_ERROR != nRet)
	{
		FILE_ERROR("initialized the socketlibs failed...");
		return FALSE;
	}
	FILE_INFOS("initialized the socketlibs successful...");

	return TRUE;
}
void CBaseServer::UnloadSocketLib()
{
	WSACleanup();
}

BOOL CBaseServer::CreateMessageDealerThread()
{
	m_hMessgaeDealerHandle = (HANDLE)_beginthreadex(NULL, 0, messageDealerFunc, this, 0, &m_uiMessageDealerThreadId);
	return TRUE;
}

unsigned int CBaseServer::heartbeatFunc(LPVOID pParam)
{
	CBaseServer* pServer = (CBaseServer*)pParam;

	//while (TRUE)
	while (WAIT_OBJECT_0 != (::WaitForSingleObject(pServer->m_hShutdownEvent, 0)))
	{
		if (0 == (pServer->m_nTick % 10))
		{
			CONSOLE_INFOS("ticks:%4d detected pluse...current connections:%d", pServer->m_nTick, pServer->getConnections());
			FILE_INFOS("ticks:%4d detected pluse...current connections:%d", pServer->m_nTick, pServer->getConnections());
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
unsigned int CBaseServer::messageDealerFunc(LPVOID pParam)
{
	CBaseServer* pServer = (CBaseServer*)pParam;
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);//强制该线程创建一个线程内消息队列，防止第一个PostThreadMessage投递失败
	if (!SetEvent(pServer->m_hMessageDealerEvent))
	{
		return 1;
	}
	while (WAIT_OBJECT_0 != (::WaitForSingleObject(pServer->m_hShutdownEvent, 0)))
	{
		switch (msg.message)
		{
		case WM_DATA_TO_SEND:
			break;
		case WM_DATA_TO_RECV:
			break;
		default:
			break;
		}
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
		CloseClients(socket);
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

BOOL CBaseServer::SendRequest(SOCKET client, LPMESSAGE_HEAD lpMessageHead, LPMESSAGE_CONTENT lpMessageContent)
{
	LPIO_CONTEXT pIoContext = new IO_CONTEXT;
	ZeroMemory(&pIoContext->m_Overlapped, sizeof(OVERLAPPED));
	ZeroMemory(&pIoContext->m_szBuffer, DATA_BUF_SIZE);
	pIoContext->m_WsaBuf.len = DATA_BUF_SIZE;
	pIoContext->m_Socket = INVALID_SOCKET;

	CBufferEx myDataPool;
	myDataPool.Write((PBYTE)lpMessageHead, sizeof(MESSAGE_HEAD));
	myDataPool.Write((PBYTE)lpMessageContent, sizeof(MESSAGE_CONTENT));

	pIoContext->m_WsaBuf.len = myDataPool.GetLength();
	memcpy(pIoContext->m_WsaBuf.buf, myDataPool.c_Bytes(), myDataPool.GetLength());

	LPSOCKET_CONTEXT pSocketContext = getClientSocketContext(client);
	if (!pSocketContext)
	{
		return FALSE;
	}
	m_IocpSocket->DoSend(pSocketContext, pIoContext, this);

	return TRUE;
}
BOOL CBaseServer::SendResponse(SOCKET client, LPMESSAGE_HEAD lpMessageHead, LPMESSAGE_CONTENT lpMessageContent)
{
	LPIO_CONTEXT pIoContext = new IO_CONTEXT;
	ZeroMemory(&pIoContext->m_Overlapped, sizeof(OVERLAPPED));
	ZeroMemory(&pIoContext->m_szBuffer, DATA_BUF_SIZE);
	pIoContext->m_WsaBuf.len = DATA_BUF_SIZE;
	pIoContext->m_Socket = INVALID_SOCKET;

	CBufferEx myDataPool;
	myDataPool.Write((PBYTE)lpMessageHead, sizeof(MESSAGE_HEAD));
	myDataPool.Write((PBYTE)lpMessageContent, sizeof(MESSAGE_CONTENT));

	pIoContext->m_WsaBuf.len = myDataPool.GetLength();
	memcpy(pIoContext->m_WsaBuf.buf, myDataPool.c_Bytes(), myDataPool.GetLength());

	LPSOCKET_CONTEXT pSocketContext = getClientSocketContext(client);
	if (!pSocketContext)
	{
		return FALSE;
	}
	m_IocpSocket->DoSend(pSocketContext, pIoContext, this);

	return TRUE;
}
void CBaseServer::OnRequest(void* pParam1, void* pParam2)
{
	LPMESSAGE_HEAD lpMessageHead = (LPMESSAGE_HEAD)pParam1;
	LPMESSAGE_CONTENT lpMessageContent = (LPMESSAGE_CONTENT)pParam2;

	switch (lpMessageContent->nRequest)
	{
	case PROTOCOL_CLIENT_PLUSE:
		CONSOLE_INFOS("client:%d socket:%d send pluse request...", lpMessageHead->lTokenID, lpMessageHead->hSocket);
		FILE_INFOS("client:%d socket:%d send pluse request...", lpMessageHead->lTokenID, lpMessageHead->hSocket);
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

void CBaseServer::CloseClients(SOCKET scoSocket)
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
	StringToTchar(strRes, szIniFile);
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
LPSOCKET_CONTEXT CBaseServer::getClientSocketContext(SOCKET client)
{
	CAutoLock lock(&m_csVectClientContext);
	unsigned int i = 0;
	for (; i < m_vectClientConetxt.size(); i++)
	{
		if (client == m_vectClientConetxt.at(i)->m_Socket)
		{
			return m_vectClientConetxt.at(i);
		}
	}

	return NULL;
}