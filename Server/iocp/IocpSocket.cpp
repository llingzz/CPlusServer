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
		FILE_ERROR("%s post the WSARecv failed...", __FUNCTION__);
		return FALSE;
	}
	FILE_INFOS("%s post WSARecv successful...", __FUNCTION__);

	return TRUE;
}
void CIOCPSocket::DoRecv(LPIO_CONTEXT pIoContext, CBaseServer* pServer)
{
	//处理来自客户端的所有消息请求
	MESSAGE_HEAD stuHead = { 0 };
	MESSAGE_CONTENT stuData = { 0 };
	//printf("Recv Data : %s\n", pIoContext->m_WsaBuf.buf);
	parseNetBuffer(pIoContext, stuHead, stuData, pServer);
	pServer->OnRequest(&stuHead, &stuData);
	//继续投递下一个Recv接受数据
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
		FILE_ERROR("%s post the WSASend failed...", __FUNCTION__);
		return FALSE;
	}
	FILE_INFOS("%s post the WSASend success...", __FUNCTION__);

	return TRUE;
}
void CIOCPSocket::DoSend(LPSOCKET_CONTEXT pSocketContext, LPIO_CONTEXT pIoContext, CBaseServer* pServer)
{
	FILE_INFOS("%s send data to ip:%s port:%d...", __FUNCTION__, inet_ntoa(pSocketContext->m_SockAddrIn.sin_addr), ntohs(pSocketContext->m_SockAddrIn.sin_port));
	PostSend(pIoContext, pServer);
}

void CIOCPSocket::OnMessageHandle(OPE_TYPE enOPEType, LPSOCKET_CONTEXT pSocketContext, LPIO_CONTEXT pIoContext, CBaseServer* pServer)
{
	switch (enOPEType)
	{
	case OPE_SEND:
		break;
	case OPE_RECV:
		DoRecv(pIoContext, pServer);
		break;
	default:
		break;
	}
}

void CIOCPSocket::parseNetBuffer(LPIO_CONTEXT pIoContext, MESSAGE_HEAD& stuHead, MESSAGE_CONTENT& stuData, CBaseServer* pServer)
{
	//客户端向服务端发送请求时，统一数据格式MESSAGE_HEAD + MESSAGE_CONTENT
	LPMESSAGE_HEAD lpMessageHead = LPMESSAGE_HEAD(PBYTE(pIoContext->m_WsaBuf.buf));
	stuHead.hSocket = pIoContext->m_Socket;
	stuHead.lSession = lpMessageHead->lSession;
	stuHead.lTokenID = lpMessageHead->lTokenID;
	LPMESSAGE_CONTENT lpMessageContent = LPMESSAGE_CONTENT(PBYTE(pIoContext->m_WsaBuf.buf + sizeof(MESSAGE_HEAD)));
	stuData.nRequest = lpMessageContent->nRequest;
	stuData.nDataLen = lpMessageContent->nDataLen;
	/*char* pDataPtr = (char*)(PBYTE(pIoContext->m_WsaBuf.buf + sizeof(MESSAGE_HEAD)+sizeof(MESSAGE_CONTENT)));
	stuData.pDataPtr = pDataPtr;*/
	//stuData.pDataPtr = (char*)(PBYTE(pIoContext->m_WsaBuf.buf + sizeof(MESSAGE_HEAD)+sizeof(MESSAGE_CONTENT)));
	memcpy(stuData.pDataPtr, (PBYTE(pIoContext->m_WsaBuf.buf + sizeof(MESSAGE_HEAD)+sizeof(MESSAGE_CONTENT))), sizeof(char*));

	//通过线程通信将解析后得到的数据投递到CBaseServer上去
	//if (!PostThreadMessage(pServer->m_uiMessageDealerThreadId, WM_DATA_TO_RECV, (WPARAM)lpMessageHead, (LPARAM)lpMessageContent))
	//{
	//	FILE_ERROR("%s postthreadmessage failed...", __FUNCTION__);
	//	//pServer->OnRequest(lpMessageHead, lpMessageContent);
	//}

	FILE_INFOS("%s %d:%d:%d---%d:%d:%s...", __FUNCTION__, stuHead.hSocket, stuHead.lSession, stuHead.lTokenID, stuData.nRequest, stuData.nDataLen, stuData.pDataPtr);
}