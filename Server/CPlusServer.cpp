#include "stdafx.h"

CPlusServer::CPlusServer()
{
	::CoInitialize(NULL);
	GetPrivateProfileString(_T("Server"), _T("ip"), _T(""), m_szIp, IP_LENGTH, GetIniFilePath());
	m_nPort = GetPrivateProfileInt(_T("Server"), _T("port"), 0, GetIniFilePath());
	m_hShutdownEvent = NULL;
	m_hMessageDealerEvent = NULL;
	m_pListenContext = NULL;
	m_pFnAcceptEx = NULL;
	m_pFnGetAcceptExSockAddr = NULL;

	m_hShutdownEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	CreateThreads();
}
CPlusServer::~CPlusServer()
{
	SAFE_RELEASE_HANDLE(m_hShutdownEvent);
	for (auto i = 0; i < m_nWorkerThreads; i++)
	{
		SAFE_RELEASE_HANDLE(m_phWorkerThread[i]);
	}
	SAFE_RELEASE_HANDLE(m_hIocp);
	SAFE_DELETE(m_pListenContext);
}

BOOL CPlusServer::Initialize()
{
	WSADATA wsaData;
	auto nRet = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (NO_ERROR != nRet)
	{
		FILE_ERROR("%s initialize the socketlibs failed...", __FUNCTION__);
		return FALSE;
	}

	if (!InitializeIocp())
	{
		FILE_ERROR("%s initialize the iocp failed...", __FUNCTION__);
		return FALSE;
	}

	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	GUID GuidGetAcceptExSockAddr = WSAID_GETACCEPTEXSOCKADDRS;

	m_pListenContext = new SOCKET_CONTEXT;
	m_pListenContext->m_Socket = INVALID_SOCKET;
	memset(&m_pListenContext->m_SockAddrIn, 0, sizeof(m_pListenContext->m_SockAddrIn));
	m_pListenContext->m_Socket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == m_pListenContext->m_Socket)
	{
		FILE_ERROR("%s initialize the WSASocket failed...", __FUNCTION__);
		return FALSE;
	}
	if (NULL == ::CreateIoCompletionPort((HANDLE)m_pListenContext->m_Socket, (HANDLE)m_hIocp, (DWORD)m_pListenContext, 0))
	{
		FILE_ERROR("%s bind the clientcontext to iocompletionport failed...", __FUNCTION__);
		SAFE_RELEASE_SOCKET(m_pListenContext->m_Socket);
		return FALSE;
	}

	sockaddr_in socketAddr;
	memset(&socketAddr, 0, sizeof(sockaddr_in));
	socketAddr.sin_family = AF_INET;
	char szTemp[IP_LENGTH] = { 0 };
	TcharToChar(m_szIp, szTemp);
	socketAddr.sin_addr.S_un.S_addr = inet_addr(szTemp);
	socketAddr.sin_port = htons((u_short)m_nPort);

	nRet = ::bind(m_pListenContext->m_Socket, (struct sockaddr*)&socketAddr, sizeof(socketAddr));
	if (SOCKET_ERROR == nRet)
	{
		FILE_ERROR("%s bind socket failed...", __FUNCTION__);
		return FALSE;
	}
	nRet = ::listen(m_pListenContext->m_Socket, SOMAXCONN);
	if (SOCKET_ERROR == nRet)
	{
		FILE_ERROR("%s listen socket failed...", __FUNCTION__);
		return FALSE;
	}

	//使用AcceptEx函数，获取函数指针
	DWORD dwBytes = 0;
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
		FILE_ERROR("%s WSAIoctl do not get the AcceptEx pointer...", __FUNCTION__);
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
		FILE_ERROR("%s WSAIoctl do not get the GetAcceptExSockAddr pointer...", __FUNCTION__);
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

		if (FALSE == PostAccept(pAcceptIoContext))
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
			FILE_ERROR("%s post accept failed...", __FUNCTION__);
			return FALSE;
		}
		//FILE_INFOS("%s posted AcceptEx request...", __FUNCTION__);
	}

	return TRUE;
}

BOOL CPlusServer::Shutdown()
{
	if (NULL != m_pListenContext && INVALID_SOCKET == m_pListenContext->m_Socket)
	{
		SetEvent(m_hShutdownEvent);
		for (auto i = 0; i < m_nWorkerThreads; i++)
		{
			PostQueuedCompletionStatus(m_hIocp, 0, (DWORD)NULL, NULL);
		}
		WaitForMultipleObjects(m_nWorkerThreads, m_phWorkerThread, TRUE, INFINITE);

		CAutoLock lock(&m_csVectClientContext);
		std::vector<LPSOCKET_CONTEXT>().swap(m_vectClientConetxt);
	}
	WSACleanup();

	FILE_INFOS("%s the server shutdown !!!", __FUNCTION__);
	CONSOLE_INFOS("%s the server shutdown !!!", __FUNCTION__);
	return TRUE;
}

BOOL CPlusServer::InitializeIocp()
{
	FILE_INFOS("%s start initialize the iocpmodule...", __FUNCTION__);
	m_hIocp = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (NULL == m_hIocp)
	{
		FILE_ERROR("%s create iocp failed...", __FUNCTION__);
		return FALSE;
	}

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	m_nWorkerThreads = MAX_WORKER_THREADS_PER_PROCESS * systemInfo.dwNumberOfProcessors;
	m_phWorkerThread = new HANDLE[m_nWorkerThreads];

	unsigned int uiThreadId = 0;
	for (auto i = 0; i < m_nWorkerThreads; i++)
	{
		LPWORKER_THREAD_PARAM pWorkerThreadParam = new WORKER_THREAD_PARAM;
		pWorkerThreadParam->pServer = this;
		pWorkerThreadParam->nThreadId = i;

		m_phWorkerThread[i] = (HANDLE)::_beginthreadex(NULL, 0, WorkerThreadFunc, (LPVOID)pWorkerThreadParam, 0, &uiThreadId);
		if (NULL == m_phWorkerThread[i])
		{
			FILE_ERROR("%s create workerthread failed at threadId :%d :%d", __FUNCTION__, uiThreadId, pWorkerThreadParam->nThreadId);
			return FALSE;
		}
	}

	FILE_INFOS("%s create %d workerthread successful...", __FUNCTION__, m_nWorkerThreads);
	FILE_INFOS("%s initialize the iocpmodule successful...", __FUNCTION__);
	return TRUE;
}

BOOL CPlusServer::OnClientPluse(LPMESSAGE_HEAD lpMessageHead, LPMESSAGE_CONTENT lpMessageContent)
{
	if (!lpMessageHead || !lpMessageContent)
	{
		FILE_ERROR("%s the params is null...", __FUNCTION__);
		return FALSE;
	}

	auto socket = lpMessageHead->hSocket;

	PLUSE_PACKAGE stuPlusePackage = { 0 };
	stuPlusePackage.m_Socket = socket;
	stuPlusePackage.m_tLastTick = time(NULL);

	if (FindClientPluse(socket, stuPlusePackage))
	{
		CAutoLock lock(&m_csMapClientPluse);
		std::map<SOCKET, PLUSE_PACKAGE>::iterator iter = m_mapClientPluse.find(socket);
		if (iter != m_mapClientPluse.end())
		{
			m_mapClientPluse.erase(iter);
			m_mapClientPluse.insert(std::make_pair(socket, stuPlusePackage));
		}
	}
	else
	{
		CAutoLock lock(&m_csMapClientPluse);
		m_mapClientPluse.insert(std::make_pair(socket, stuPlusePackage));
	}

	FILE_INFOS("%s on got client pluse data [%s]...", __FUNCTION__, lpMessageContent->pDataPtr);
	CONSOLE_INFOS("%s on got client pluse data [%s]...", __FUNCTION__, lpMessageContent->pDataPtr);

	return TRUE;
}

BOOL CPlusServer::PostAccept(LPIO_CONTEXT pIoContext)
{
	if (INVALID_SOCKET == m_pListenContext->m_Socket)
	{
		FILE_ERROR("%s the socket is invalid...", __FUNCTION__);
		return FALSE;
	}

	DWORD dwBytes = 0;
	pIoContext->m_OpeType = OPE_ACCEPT;
	WSABUF* wsaBuf = &(pIoContext->m_WsaBuf);
	OVERLAPPED* pOverlapped = &(pIoContext->m_Overlapped);
	//为之后新连入的客户端先准备好Socket
	pIoContext->m_Socket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == pIoContext->m_Socket)
	{
		FILE_ERROR("%s create the scoket for accept failed...", __FUNCTION__);
		return FALSE;
	}

	auto bRet = m_pFnAcceptEx(m_pListenContext->m_Socket, pIoContext->m_Socket, wsaBuf->buf, /*wsaBuf->len - (sizeof(sockaddr_in)+ACCEPTEX_BYTES_OFFSET_SIZE) * 2*/0,
		sizeof(sockaddr_in)+ACCEPTEX_BYTES_OFFSET_SIZE, sizeof(sockaddr_in)+ACCEPTEX_BYTES_OFFSET_SIZE, &dwBytes, pOverlapped);
	if (FALSE == bRet)
	{
		if (WSA_IO_PENDING != WSAGetLastError())
		{
			FILE_ERROR("%s post the AcceptEx request filed...%s", __FUNCTION__, WSAGetLastError());
			return FALSE;
		}
		FILE_INFOS("%s post the AcceptEx request successful with m_pFnAcceptEx == FALSE...", __FUNCTION__);
	}
	
	FILE_INFOS("%s post the AcceptEx request successful...", __FUNCTION__);
	return TRUE;
}

BOOL CPlusServer::DoAccept(LPSOCKET_CONTEXT pSocketContext, LPIO_CONTEXT pIoContext)
{
	sockaddr_in* siClientAddr = NULL;
	sockaddr_in* siServerAddr = NULL;

	auto nSiClientLen = (int)sizeof(sockaddr_in);
	auto nSiServerLen = (int)sizeof(sockaddr_in);

	m_pFnGetAcceptExSockAddr(
		pIoContext->m_WsaBuf.buf,
		/*pIoContext->m_WsaBuf.len - ((sizeof(sockaddr_in)+ACCEPTEX_BYTES_OFFSET_SIZE) * 2)*/0,
		sizeof(sockaddr_in)+ACCEPTEX_BYTES_OFFSET_SIZE,
		sizeof(sockaddr_in)+ACCEPTEX_BYTES_OFFSET_SIZE,
		(LPSOCKADDR*)&siServerAddr,
		&nSiServerLen,
		(LPSOCKADDR*)&siClientAddr,
		&nSiClientLen);

	FILE_INFOS("client connected with ip:%s port:%d...", inet_ntoa(siClientAddr->sin_addr), ntohs(siClientAddr->sin_port));
	CONSOLE_INFOS("client connected with ip:%s port:%d...", inet_ntoa(siClientAddr->sin_addr), ntohs(siClientAddr->sin_port));

	//接着新建一个SOCKET_CONTEXT用于新连入的Socket，保留原来的Context用于监听下一个连接
	LPSOCKET_CONTEXT pNewSocketContext = new SOCKET_CONTEXT;
	pNewSocketContext->m_Socket = INVALID_SOCKET;
	ZeroMemory(&pNewSocketContext->m_SockAddrIn, sizeof(pNewSocketContext->m_SockAddrIn));
	pNewSocketContext->m_vectIoContext.clear();

	pNewSocketContext->m_Socket = pIoContext->m_Socket;
	memcpy(&pNewSocketContext->m_SockAddrIn, siClientAddr, sizeof(sockaddr_in));

	//与完成端口绑定
	if (NULL == (::CreateIoCompletionPort((HANDLE)pNewSocketContext->m_Socket, m_hIocp, (DWORD)pNewSocketContext, 0)))
	{
		FILE_ERROR("%s associate with the iocompletionport failed...", __FUNCTION__);
		SAFE_DELETE(pNewSocketContext);
		return FALSE;
	}
	//接着新建一个IO_CONTEXT，用于在这个Socket投递第一个IO请求(这里为Recv操作)
	LPIO_CONTEXT pNewIoContext = new IO_CONTEXT;
	ZeroMemory(&pNewIoContext->m_Overlapped, sizeof(OVERLAPPED));
	ZeroMemory(&pNewIoContext->m_szBuffer, DATA_BUF_SIZE);
	pNewIoContext->m_WsaBuf.buf = pNewIoContext->m_szBuffer;
	pNewIoContext->m_WsaBuf.len = DATA_BUF_SIZE;
	pNewIoContext->m_Socket = INVALID_SOCKET;

	pNewIoContext->m_OpeType = OPE_RECV;
	pNewIoContext->m_Socket = pNewSocketContext->m_Socket;

	pNewSocketContext->m_vectIoContext.push_back(pNewIoContext);
	if (FALSE == PostRecv(pNewIoContext))
	{
		FILE_ERROR("%s post new iocontext failed...", __FUNCTION__);
		std::vector<LPIO_CONTEXT>::iterator iter;
		for (iter = pNewSocketContext->m_vectIoContext.begin(); iter != pNewSocketContext->m_vectIoContext.end();)
		{
			if (*iter == pNewIoContext)
			{
				delete pNewIoContext;
				pNewIoContext = NULL;
				iter = pNewSocketContext->m_vectIoContext.erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}
	//投递成功的话就将这个有效的客户端信息添加到m_vectClientContext
	{
		CAutoLock lock(&m_csVectClientContext);
		m_vectClientConetxt.push_back(pNewSocketContext);
	}
	//客户端第一次投递的消息会从这里得到，在这里解析一下，post到线程消息队列中去
	//HandleNetMessage(pIoContext, WM_DATA_TO_RECV);
	//重置ListenSocket上面的IoContext，用于准备投递新的AccepEx
	ZeroMemory(pIoContext->m_szBuffer, DATA_BUF_SIZE);

	return PostAccept(pIoContext);
}

BOOL CPlusServer::PostSend(LPIO_CONTEXT pIoContext)
{
	DWORD dwFlags = 0;
	DWORD dwBytes = 0;
	WSABUF* pWSABuf = &(pIoContext->m_WsaBuf);
	OVERLAPPED* pOverlapped = &(pIoContext->m_Overlapped);
	pIoContext->m_OpeType = OPE_SEND;
	//投递WSASend请求
	auto nRet = ::WSASend(pIoContext->m_Socket, pWSABuf, 1, &dwBytes, dwFlags, pOverlapped, NULL);
	if ((SOCKET_ERROR == nRet) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		FILE_ERROR("%s post the WSASend failed...", __FUNCTION__);
		return FALSE;
	}
	
	//FILE_INFOS("%s post the WSASend success...", __FUNCTION__);
	return TRUE;
}

BOOL CPlusServer::DoSend(LPSOCKET_CONTEXT pSocketContext, LPIO_CONTEXT pIoContext)
{
	FILE_INFOS("%s send data to ip:%s port:%d...", __FUNCTION__, inet_ntoa(pSocketContext->m_SockAddrIn.sin_addr), ntohs(pSocketContext->m_SockAddrIn.sin_port));
	return PostSend(pIoContext);
}

BOOL CPlusServer::PostRecv(LPIO_CONTEXT pIoContext)
{
	DWORD dwFlags = 0;
	DWORD dwBytes;
	ZeroMemory(&(pIoContext->m_Overlapped), sizeof(OVERLAPPED));
	pIoContext->m_WsaBuf.len = DATA_BUF_SIZE;
	pIoContext->m_WsaBuf.buf = pIoContext->m_szBuffer;
	pIoContext->m_OpeType = OPE_RECV;
	//投递WSARecv请求
	auto nRet = ::WSARecv(pIoContext->m_Socket, &(pIoContext->m_WsaBuf), 1, &dwBytes, &dwFlags, &(pIoContext->m_Overlapped), NULL);
	if (SOCKET_ERROR == nRet && WSA_IO_PENDING != GetLastError())
	{
		//如果返回错误而且错误代码并非是Pending的话，请求失败
		FILE_ERROR("%s post the WSARecv failed...", __FUNCTION__);
		return FALSE;
	}

	//FILE_INFOS("%s post WSARecv successful...", __FUNCTION__);
	return TRUE;
}

BOOL CPlusServer::DoRecv(LPIO_CONTEXT pIoContext)
{
	HandleNetMessage(pIoContext, WM_DATA_TO_RECV);
	//继续投递下一个Recv接受数据
	return PostRecv(pIoContext);
}

BOOL CPlusServer::SendRequest(SOCKET sClient, int nRequest, void* pDataPtr, int nDataLen)
{
	auto pMessageHead = new MESSAGE_HEAD;
	pMessageHead->hSocket = sClient;
	auto pMessageContent = new MESSAGE_CONTENT;
	pMessageContent->nRequest = nRequest;
	pMessageContent->nDataLen = nDataLen;
	pMessageContent->pDataPtr = new BYTE[nDataLen];
	memcpy(pMessageContent->pDataPtr, pDataPtr, nDataLen);
	
	while (!PostThreadMessage(m_uiMessageDealerThreadId, WM_DATA_TO_SEND, (WPARAM)pMessageHead, (LPARAM)pMessageContent))
	{
		FILE_ERROR("%s post the thread message failed with nRequestID:%d", __FUNCTION__, pMessageContent->nRequest);
		return FALSE;
	}

	return TRUE;
}

BOOL CPlusServer::SendResponse(SOCKET sClient, int nRequest, void* pDataPtr, int nDataLen)
{
	auto pMessageHead = new MESSAGE_HEAD;
	pMessageHead->hSocket = sClient;
	auto pMessageContent = new MESSAGE_CONTENT;
	pMessageContent->nRequest = nRequest;
	pMessageContent->nDataLen = nDataLen;
	pMessageContent->pDataPtr = new BYTE[nDataLen];
	memcpy(pMessageContent->pDataPtr, pDataPtr, nDataLen);

	while (!PostThreadMessage(m_uiMessageDealerThreadId, WM_DATA_TO_SEND, (WPARAM)pMessageHead, (LPARAM)pMessageContent))
	{
		FILE_ERROR("%s post the thread message failed with nRequestID:%d", __FUNCTION__, pMessageContent->nRequest);
		return FALSE;
	}

	return TRUE;
}

BOOL CPlusServer::OnRequest(void* pParam1, void* pParam2)
{
	if (!pParam1 || !pParam2)
	{
		FILE_ERROR("%s the params is NULL!!!", __FUNCTION__);
		return FALSE;
	}
	
	LPMESSAGE_HEAD lpMessageHead = (LPMESSAGE_HEAD)pParam1;
	LPMESSAGE_CONTENT lpMessageContent = (LPMESSAGE_CONTENT)pParam2;
	if (!lpMessageHead || !lpMessageContent)
	{
		FILE_ERROR("%s the deserialize params is NULL!!!", __FUNCTION__);
		return FALSE;
	}
	
	auto nTicksCounts = GetTickCount();
	CONSOLE_INFOS("------ begin the request id %ld  ------", lpMessageContent->nRequest);
	CONSOLE_INFOS("------ the socket %ld requesting ------", lpMessageHead->hSocket);

	switch (lpMessageContent->nRequest)
	{
	case PROTOCOL_CLIENT_PLUSE:
		CONSOLE_INFOS("%s socket %ld push PROTOCOL_CLIENT_PLUSE requesting", __FUNCTION__, lpMessageHead->hSocket);
		FILE_INFOS("%s socket %ld push PROTOCOL_CLIENT_PLUSE requesting", __FUNCTION__, lpMessageHead->hSocket);
		OnClientPluse(lpMessageHead, lpMessageContent);
		break;
	default:
		CONSOLE_INFOS("%s socket %ld push NULL requesting", __FUNCTION__, lpMessageHead->hSocket);
		FILE_INFOS("%s socket %ld push NULL requesting", __FUNCTION__, lpMessageHead->hSocket);
		break;
	}

	CONSOLE_INFOS("------ end the request id %ld cost time %ld ------", lpMessageContent->nRequest, GetTickCount() - nTicksCounts);

	SAFE_DELETE(lpMessageHead);
	SAFE_DELETE(lpMessageContent->pDataPtr);
	SAFE_DELETE(lpMessageContent);
	return TRUE;
}

BOOL CPlusServer::OnResponse(void* pParam1, void* pParam2)
{
	if (!pParam1 || !pParam2)
	{
		FILE_ERROR("%s the params is NULL!!!", __FUNCTION__);
		return FALSE;
	}

	LPMESSAGE_HEAD lpMessageHead = (LPMESSAGE_HEAD)pParam1;
	LPMESSAGE_CONTENT lpMessageContent = (LPMESSAGE_CONTENT)pParam2;
	if (!lpMessageHead || !lpMessageContent)
	{
		FILE_ERROR("%s the deserialize params is NULL!!!", __FUNCTION__);
		return FALSE;
	}

	CBufferEx myDataPool;
	SerializeNetMessage(myDataPool, lpMessageHead, lpMessageContent, lpMessageContent->pDataPtr, lpMessageContent->nDataLen);
	SendData(lpMessageHead->hSocket, (void*)myDataPool.c_Bytes(), myDataPool.GetLength());
	
	SAFE_DELETE(lpMessageHead);
	SAFE_DELETE(lpMessageContent->pDataPtr);
	SAFE_DELETE(lpMessageContent);
	return TRUE;
}

BOOL CPlusServer::SendData(SOCKET sClient, void* pDataPtr, int nDataLen)
{
	LPSOCKET_CONTEXT pSocketContext = new SOCKET_CONTEXT;
	FindClientSocket(sClient, *pSocketContext);
	LPIO_CONTEXT pIoContext = new IO_CONTEXT;
	pIoContext->m_WsaBuf.len = nDataLen;
	pIoContext->m_WsaBuf.buf = new CHAR[nDataLen];
	memcpy(pIoContext->m_WsaBuf.buf, pDataPtr, nDataLen);

	return DoSend(pSocketContext, pIoContext);
}

BOOL CPlusServer::SimulateRequest(int nRequest, void* pDataPtr, int nDataLen)
{
	LPMESSAGE_HEAD pMessageHead = new MESSAGE_HEAD;
	LPMESSAGE_CONTENT pMessageContent = new MESSAGE_CONTENT;
	pMessageContent->nRequest = nRequest;
	pMessageContent->nDataLen = nDataLen;
	pMessageContent->pDataPtr = new BYTE[nDataLen];
	memcpy(pMessageContent->pDataPtr, pDataPtr, nDataLen);

	while (!PostThreadMessage(m_uiMessageDealerThreadId, WM_DATA_TO_RECV, (WPARAM)pMessageHead, (LPARAM)pMessageContent))
	{
		FILE_ERROR("%s post the thread message failed.", __FUNCTION__);
		return FALSE;
	}
	return TRUE;
}

BOOL CPlusServer::HandleNetMessage(LPIO_CONTEXT pIoContext, int nOpeType)
{
	auto pMessageHead = new MESSAGE_HEAD;
	auto pMessageContent = new MESSAGE_CONTENT;
	DeserializeNetMessage(pIoContext, pMessageHead, pMessageContent);
	while (!PostThreadMessage(m_uiMessageDealerThreadId, nOpeType, (WPARAM)pMessageHead, (LPARAM)pMessageContent))
	{
		FILE_ERROR("%s post the thread message failed with nRequestID:%d retrying...", __FUNCTION__, pMessageContent->nRequest);
	}

	return TRUE;
}

void CPlusServer::CreateThreads()
{
	m_hMessageDealerEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hMessgaeDealerHandle = (HANDLE)::_beginthreadex(NULL, 0, DealerThreadFunc, this, 0, &m_uiMessageDealerThreadId);
	m_hClientPluseHandle = (HANDLE)::_beginthreadex(NULL, 0, ClientPluseFunc, this, 0, &m_uiClientPluseThreadId);
}

void CPlusServer::CloseClients(SOCKET scoSocket)
{
	CAutoLock lock(&m_csVectClientContext);
	std::vector<LPSOCKET_CONTEXT>::iterator iter = m_vectClientConetxt.begin();
	while (iter != m_vectClientConetxt.end())
	{
		if (scoSocket == (*iter)->m_Socket)
		{
			m_vectClientConetxt.erase(iter);
			break;
		}
		iter++;
	}
	::closesocket(scoSocket);
}

void CPlusServer::RemoveSocketContext(LPSOCKET_CONTEXT pSocketContext)
{
	CAutoLock lock(&m_csVectClientContext);
	std::vector<LPSOCKET_CONTEXT>::iterator iter;
	for (iter = m_vectClientConetxt.begin(); iter != m_vectClientConetxt.end();)
	{
		if (*iter == pSocketContext)
		{
			delete pSocketContext;
			pSocketContext = NULL;
			iter = m_vectClientConetxt.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

void CPlusServer::SerializeNetMessage(CBufferEx& dataBuffer, LPMESSAGE_HEAD lpMessageHead, LPMESSAGE_CONTENT lpMessageContent, void* pData, int nLen)
{
	dataBuffer.Write((PBYTE)lpMessageHead, sizeof(MESSAGE_HEAD));
	dataBuffer.Write((PBYTE)lpMessageContent, sizeof(MESSAGE_CONTENT));
	dataBuffer.Write((PBYTE)pData, nLen);
}

void CPlusServer::DeserializeNetMessage(LPIO_CONTEXT pIoContext, LPMESSAGE_HEAD pMessageHead, LPMESSAGE_CONTENT pMessageContent)
{
	memcpy(pMessageHead, PBYTE(pIoContext->m_WsaBuf.buf), sizeof(MESSAGE_HEAD));
	memcpy(pMessageContent, PBYTE(pIoContext->m_WsaBuf.buf + sizeof(MESSAGE_HEAD)), sizeof(MESSAGE_CONTENT));
	pMessageContent->pDataPtr = new BYTE[pMessageContent->nDataLen];
	memcpy(pMessageContent->pDataPtr, PBYTE(pIoContext->m_WsaBuf.buf + sizeof(MESSAGE_HEAD)+sizeof(MESSAGE_CONTENT)), pMessageContent->nDataLen);
}

BOOL CPlusServer::FindClientSocket(SOCKET sSocket, SOCKET_CONTEXT& rSocketContext)
{
	CAutoLock lock(&m_csVectClientContext);
	std::vector<LPSOCKET_CONTEXT>::iterator iter = m_vectClientConetxt.begin();
	while (iter != m_vectClientConetxt.end())
	{
		if (sSocket == (*iter)->m_Socket)
		{
			rSocketContext.m_Socket = (*iter)->m_Socket;
			rSocketContext.m_SockAddrIn = (*iter)->m_SockAddrIn;
			rSocketContext.m_vectIoContext = (*iter)->m_vectIoContext;

			return TRUE;
		}
		iter++;
	}
	return FALSE;
}

BOOL CPlusServer::FindClientPluse(SOCKET sSocket, PLUSE_PACKAGE& rPlusePackage)
{
	CAutoLock lock(&m_csMapClientPluse);
	std::map<SOCKET, PLUSE_PACKAGE>::iterator iter = m_mapClientPluse.find(sSocket);
	if (iter != m_mapClientPluse.end())
	{
		rPlusePackage = iter->second;
		return TRUE;
	}

	return FALSE;
}

LPCTSTR CPlusServer::GetIniFilePath()
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
	strRes.append("\\");
	strRes.append(SERVER_NAME);
	strRes.append(".ini");
	StringToTchar(strRes, szIniFile);
	lstrcpy(m_szIniFilePath, szIniFile);

	return m_szIniFilePath;
}

unsigned __stdcall CPlusServer::WorkerThreadFunc(LPVOID lpParam)
{
	LPWORKER_THREAD_PARAM pWorkerThreadParam = (LPWORKER_THREAD_PARAM)lpParam;
	FILE_INFOS("%s the workerthread with id:%d started...", __FUNCTION__, pWorkerThreadParam->nThreadId);

	OVERLAPPED* pOverlapped = NULL;
	LPSOCKET_CONTEXT pSocketContext = NULL;
	DWORD dwByteTransfered = 0;
	while (WAIT_OBJECT_0 != WaitForSingleObject(pWorkerThreadParam->pServer->m_hShutdownEvent, 0))
	{
		auto bRet = ::GetQueuedCompletionStatus(
			pWorkerThreadParam->pServer->m_hIocp,
			&dwByteTransfered,
			(PULONG_PTR)&pSocketContext,
			&pOverlapped,
			INFINITE);
		if (!bRet)
		{
			if (NULL == pSocketContext)
			{
				FILE_ERROR("%s GetQueuedCompletionStatus got socketcontext is NULL!!!", __FUNCTION__);
				continue;
			}
			DWORD dwError = GetLastError();
			if (ERROR_INVALID_HANDLE == dwError)
			{
				FILE_ERROR("%s the handle is invalid... dwError:%d ", __FUNCTION__, dwError);
				break;
			}
			else if (ERROR_OPERATION_ABORTED == dwError)
			{
				FILE_ERROR("%s the I/O operation has been aborted because of either a thread exit or an application request. dwError:%d ", __FUNCTION__, dwError);
				FILE_ERROR("%s client with ip:%s port:%d disconnected...", __FUNCTION__, inet_ntoa(pSocketContext->m_SockAddrIn.sin_addr), ntohs(pSocketContext->m_SockAddrIn.sin_port));
				pWorkerThreadParam->pServer->CloseClients(pSocketContext->m_Socket);
				pWorkerThreadParam->pServer->RemoveSocketContext(pSocketContext);
			}
			else if (WAIT_TIMEOUT == dwError)
			{
				FILE_ERROR("%s the wait operation timed out... retrying... dwError:%d ", __FUNCTION__, dwError);
			}
			else if (ERROR_NETNAME_DELETED == dwError)
			{
				FILE_ERROR("%s the specified network name is no longer available. dwError:%d ", __FUNCTION__, dwError);
				FILE_ERROR("%s client with ip:%s port:%d disconnected...", __FUNCTION__, inet_ntoa(pSocketContext->m_SockAddrIn.sin_addr), ntohs(pSocketContext->m_SockAddrIn.sin_port));
				pWorkerThreadParam->pServer->CloseClients(pSocketContext->m_Socket);
				pWorkerThreadParam->pServer->RemoveSocketContext(pSocketContext);
			}
			else
			{
				FILE_ERROR("%s GetQueuedCompletionStatus failed. GetLastError() returns dwError:%d ", __FUNCTION__, dwError);
				FILE_ERROR("%s client with ip:%s port:%d disconnected...", __FUNCTION__, inet_ntoa(pSocketContext->m_SockAddrIn.sin_addr), ntohs(pSocketContext->m_SockAddrIn.sin_port));
				pWorkerThreadParam->pServer->CloseClients(pSocketContext->m_Socket);
				pWorkerThreadParam->pServer->RemoveSocketContext(pSocketContext);
			}
			continue;
		}
		else
		{
			LPIO_CONTEXT pIoContext = CONTAINING_RECORD(pOverlapped, IO_CONTEXT, m_Overlapped);
			if (0 == dwByteTransfered && (OPE_RECV == pIoContext->m_OpeType || OPE_SEND == pIoContext->m_OpeType))
			{
				FILE_ERROR("client with ip:%s: port:%d disconnected...", inet_ntoa(pSocketContext->m_SockAddrIn.sin_addr), ntohs(pSocketContext->m_SockAddrIn.sin_port));
				pWorkerThreadParam->pServer->CloseClients(pSocketContext->m_Socket);
				pWorkerThreadParam->pServer->RemoveSocketContext(pSocketContext);
				continue;
			}
			else
			{
				switch ((OPE_TYPE)pIoContext->m_OpeType)
				{
				case OPE_ACCEPT:
					//FILE_INFOS("%s OPE_ACCEPT request...", __FUNCTION__);
					pWorkerThreadParam->pServer->DoAccept(pSocketContext, pIoContext);
					break;
				case OPE_RECV:
					//FILE_INFOS("%s OPE_RECV request...", __FUNCTION__);
					pWorkerThreadParam->pServer->DoRecv(pIoContext);
					break;
				case OPE_SEND:
					//FILE_INFOS("%s OPE_SEND request...", __FUNCTION__);
					break;
				default:
					FILE_WARNS("%s pIoContext->m_OpType == default of workerthread params encounter a problem...", __FUNCTION__);
					break;
				}
			}
		}
	}

	FILE_FATAL("%s workerthread with id:%d exit...", __FUNCTION__, pWorkerThreadParam->nThreadId);
	SAFE_DELETE(lpParam);
	return 0;
}

unsigned __stdcall CPlusServer::DealerThreadFunc(LPVOID lpParam)
{
	CPlusServer* pServer = (CPlusServer*)lpParam;
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	if (!SetEvent(pServer->m_hMessageDealerEvent))
	{//强制该线程创建一个线程内消息队列，防止第一个PostThreadMessage投递失败
		return 1;
	}
	while (WAIT_OBJECT_0 != WaitForSingleObject(pServer->m_hShutdownEvent, 0))
	{
		while (GetMessage(&msg, 0, 0, 0))
		{
			switch (msg.message)
			{
			case WM_DATA_TO_SEND:
				pServer->OnResponse((LPMESSAGE_HEAD)msg.wParam, (LPMESSAGE_CONTENT)msg.lParam);
				break;
			case WM_DATA_TO_RECV:
				pServer->OnRequest((LPMESSAGE_HEAD)msg.wParam, (LPMESSAGE_CONTENT)msg.lParam);
				break;
			default:
				FILE_ERROR("%s the message type is invalid...", __FUNCTION__);
				DispatchMessage(&msg);
				break;
			}
		}
	}

	FILE_FATAL("%s thread exit...", __FUNCTION__);
	return 0;
}

unsigned __stdcall CPlusServer::ClientPluseFunc(LPVOID lpParam)
{
	CPlusServer* pServer = (CPlusServer*)lpParam;
	while (WAIT_OBJECT_0 != ::WaitForSingleObject(pServer->m_hClientPluseHandle, 0))
	{
		Sleep(MAX_PLUSE_INTEVAL * 1000);
		auto nTickCounts = GetTickCount();
		FILE_INFOS("%s start clear clients which lost connection with server beyond 30 seconds...", __FUNCTION__);
		CAutoLock lock(&pServer->m_csMapClientPluse);
		std::map<SOCKET, PLUSE_PACKAGE>::iterator iter = pServer->m_mapClientPluse.begin();
		while (iter != pServer->m_mapClientPluse.end())
		{
			auto nInteval = time(NULL) - iter->second.m_tLastTick;
			if (nInteval > (MAX_PLUSE_INTEVAL * 1000))
			{
				iter = pServer->m_mapClientPluse.erase(iter);
				pServer->CloseClients(iter->first);
			}
			iter++;
		}
		FILE_INFOS("%s clear lost clients done current connections %d cost time %ld ", __FUNCTION__, pServer->m_mapClientPluse.size(), GetTickCount() - nTickCounts);
	}

	FILE_FATAL("%s thread exit...", __FUNCTION__);
	return 0;
}