#pragma once

#define TEST_CONNECTION		10000
/************************************************************************/
/* Utilties.h                                                           */
/*CBufferPool信息*/
typedef struct _tagBUFFER_INFO {
	DWORD dwDataSize;
	DWORD dwBufferSize;
	DWORD dwDataPacketCount;
}BUFFER_INFO, *LPBUFFER_INFO;
/************************************************************************/
/************************************************************************/
/* iocp.h                                                               */
/*数据包头*/
typedef struct _tagPACKET_HEAD{
	UINT uiPacketNo;
	UINT uiMsgType;
	UINT uiPacketLen;
	BOOL bUseCRC32;
	DWORD dwCRC32;
}PACKET_HEAD, *LPPACKET_HEAD;
/*套接字上下文头信息*/
typedef struct _tagCONTEXT_HEAD{
	SOCKET hSocket;
	UINT uiTokenID;
	UINT uiSessionID;
}CONTEXT_HEAD, *LPCONTEXT_HEAD;
/*请求头文件*/
typedef struct _tagREQUEST_HEAD{
	UINT nRepeated;
	UINT nRequest;
}REQUEST_HEAD, *LPREQUEST_HEAD;
/************************************************************************/