#include "stdafx.h"

CIOCPAccept::CIOCPAccept()
{

}
CIOCPAccept::~CIOCPAccept()
{

}

BOOL CIOCPAccept::PostAccept(LPIO_CONTEXT pIoContext, CBaseServer* pServer)
{
	if (INVALID_SOCKET == pServer->m_pListenContext->m_Socket)
	{
		printf("CIOCPAccept::PostAccept The SOCKET is INVALID...\n");
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
		printf("CIOCPAccept::PostAccept Create the SOCKET for Accept Failed...\n");
		return FALSE;
	}

	auto bRet = pServer->m_pFnAcceptEx(pServer->m_pListenContext->m_Socket, pIoContext->m_Socket, wsaBuf->buf, wsaBuf->len - (sizeof(sockaddr_in)+ACCEPTEX_BYTES_OFFSET_SIZE) * 2,
		sizeof(sockaddr_in)+ACCEPTEX_BYTES_OFFSET_SIZE, sizeof(sockaddr_in)+ACCEPTEX_BYTES_OFFSET_SIZE, &dwBytes, pOverlapped);
	if (FALSE == bRet)
	{
		if (WSA_IO_PENDING != WSAGetLastError())
		{
			printf("CIOCPAccept::PostAccept Post the AcceptEx Request Failed...%s\n", WSAGetLastError());
			return FALSE;
		}
		printf("Post the AcceptEx Request Successful with m_pFnAcceptEx==FALSE...\n");
	}
	printf("Post the AcceptEx Request Successful...\n");

	return TRUE;
}
BOOL CIOCPAccept::DoAccept(LPSOCKET_CONTEXT pSocketContext, LPIO_CONTEXT pIoContext, CBaseServer* pServer)
{
	sockaddr_in* siClientAddr = NULL;
	sockaddr_in* siServerAddr = NULL;

	auto nSiClientLen = (int)sizeof(sockaddr_in);
	auto nSiServerLen = (int)sizeof(sockaddr_in);

	pServer->m_pFnGetAcceptExSockAddr(
		pIoContext->m_WsaBuf.buf,
		pIoContext->m_WsaBuf.len - ((sizeof(sockaddr_in)+ACCEPTEX_BYTES_OFFSET_SIZE) * 2),
		sizeof(sockaddr_in)+ACCEPTEX_BYTES_OFFSET_SIZE,
		sizeof(sockaddr_in)+ACCEPTEX_BYTES_OFFSET_SIZE,
		(LPSOCKADDR*)&siServerAddr,
		&nSiServerLen,
		(LPSOCKADDR*)&siClientAddr,
		&nSiClientLen);

	printf("A Client Connected with IP:%s:%d\n", inet_ntoa(siClientAddr->sin_addr), ntohs(siClientAddr->sin_port));
	printf("Got Origin Data In DoAccept:%s\n", pIoContext->m_WsaBuf.buf);

	//将新连接的客户端Socket信息保存起来
	unsigned int nTemp = pServer->m_nTick % HEART_BEAT_WHEEL_SLOT;
	unsigned int nIndex = (nTemp == 0 ? (HEART_BEAT_WHEEL_SLOT - 1) : nTemp);
	{
		CAutoLock lock(&pServer->m_csHeartBeatWheel);
		pServer->m_vectHeartBeatWheel[nIndex].push_back(pIoContext->m_Socket);
	}

	//接着新建一个SOCKET_CONTEXT用于新连入的Socket，保留原来的Context用于监听下一个连接
	LPSOCKET_CONTEXT pNewSocketContext = new SOCKET_CONTEXT;
	pNewSocketContext->m_Socket = INVALID_SOCKET;
	ZeroMemory(&pNewSocketContext->m_SockAddrIn, sizeof(pNewSocketContext->m_SockAddrIn));
	pNewSocketContext->m_vectIoContext.clear();

	pNewSocketContext->m_Socket = pIoContext->m_Socket;
	memcpy(&pNewSocketContext->m_SockAddrIn, siClientAddr, sizeof(sockaddr_in));

	//与完成端口绑定
	if (NULL == (::CreateIoCompletionPort((HANDLE)pNewSocketContext->m_Socket, pServer->m_IocpModule->m_hIocp, (DWORD)pNewSocketContext, 0)))
	{
		printf("CIOCPAccept::DoAccept Associate with the IOCP Failed...\n");
		SAFE_DELETE(pNewSocketContext);
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

	if (FALSE == pServer->m_IocpSocket->PostRecv(pNewIoContext, pServer))
	{
		printf("CIOCPAccept::DoAccept Post NewIoContext Failed...\n");

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
	CAutoLock lock(&pServer->m_csVectClientContext);
	pServer->m_vectClientConetxt.push_back(pNewSocketContext);

	//重置ListenSocket上面的IoContext，用于准备投递新的AccepEx
	ZeroMemory(pIoContext->m_szBuffer, DATA_BUF_SIZE);

	return PostAccept(pIoContext, pServer);
}

void CIOCPAccept::OnMessageHandle(OPE_TYPE enOPEType, LPSOCKET_CONTEXT pSocketContext, LPIO_CONTEXT pIoContext, CBaseServer* pServer)
{
	switch (enOPEType)
	{
	case OPE_ACCEPT:
		DoAccept(pSocketContext, pIoContext, pServer);
		break;
	default:
		break;
	}
}