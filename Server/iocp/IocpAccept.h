#pragma once

class CIOCPAccept{
public:
	CIOCPAccept();
	~CIOCPAccept();

public:
	BOOL PostAccept(LPIO_CONTEXT pIoContext, CBaseServer* pServer);
	BOOL DoAccept(LPSOCKET_CONTEXT pSocketContext, LPIO_CONTEXT pIoContext, CBaseServer* pServer);

	void OnMessageHandle(OPE_TYPE enOPEType, LPSOCKET_CONTEXT pSocketContext, LPIO_CONTEXT pIoContext, CBaseServer* pServer);
};