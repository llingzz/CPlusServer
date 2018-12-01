#include "stdafx.h"

CIOCPSocket::CIOCPSocket()
{

}
CIOCPSocket::~CIOCPSocket()
{

}

BOOL CIOCPSocket::PostRecv(LPIO_CONTEXT pIoContext, CBaseServer* pServer)
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
		//LOG_ERROR("Post the WSARecv Failed...");
		return FALSE;
	}
	//LOG_INFO("Post WSARecv Successful...");

	return TRUE;
}
void CIOCPSocket::DoRecv(LPIO_CONTEXT pIoContext, CBaseServer* pServer)
{
	//LOG_INFO("Get Data In DoRecv: " << pIoContext->m_WsaBuf.buf);

	PostRecv(pIoContext, pServer);
}

BOOL CIOCPSocket::PostSend(LPIO_CONTEXT pIoContext, CBaseServer* pServer)
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
		//LOG_ERROR("Post the WSASend Failed...");
		return FALSE;
	}
	//LOG_INFO("Post the WSASend Success...");

	return TRUE;
}
void CIOCPSocket::DoSend(LPSOCKET_CONTEXT pSocketContext, LPIO_CONTEXT pIoContext, CBaseServer* pServer)
{
	//LOG_INFO("Send Data To " << inet_ntoa(pSocketContext->m_SockAddrIn.sin_addr) << " : " << ntohs(pSocketContext->m_SockAddrIn.sin_port));

	PostSend(pIoContext, pServer);
}

void CIOCPSocket::OnMessageHandle(OPE_TYPE enOPEType, LPSOCKET_CONTEXT pSocketContext, LPIO_CONTEXT pIoContext, CBaseServer* pServer)
{
	switch (enOPEType)
	{
	case OPE_SEND:
		//DoSend(pSocketContext, pIoContext, pServer);
		break;
	case OPE_RECV:
		DoRecv(pIoContext, pServer);
		break;
	default:
		break;
	}
}