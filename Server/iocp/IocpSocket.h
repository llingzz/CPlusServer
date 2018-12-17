#pragma once

class CIOCPSocket{
public:
	CIOCPSocket();
	~CIOCPSocket();

public:
	BOOL PostRecv(LPIO_CONTEXT pIoContext, CBaseServer* pServer);
	void DoRecv(LPIO_CONTEXT pIoContext, CBaseServer* pServer);

	BOOL PostSend(LPIO_CONTEXT pIoContext, CBaseServer* pServer);
	void DoSend(LPSOCKET_CONTEXT pSocketContext, LPIO_CONTEXT pIoContext, CBaseServer* pServer);

	void OnMessageHandle(OPE_TYPE enOPEType, LPSOCKET_CONTEXT pSocketContext, LPIO_CONTEXT pIoContext, CBaseServer* pServer);

private:
	void parseNetBuffer(LPIO_CONTEXT pIoContext, MESSAGE_HEAD& stuHead, MESSAGE_CONTENT& stuData);
};