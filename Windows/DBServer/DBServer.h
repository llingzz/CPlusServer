#pragma once
#include "iocp.h"
#include "DBConnection.h"
#include "OleDBConnection.h"

class CWorkerContext {
public:
	CWorkerContext() {
		m_bReconnect = FALSE;
		//m_pConnection = new COleDBConnection;
		m_pConnection = new CDBConnection;
	}
	virtual ~CWorkerContext() {

	}

public:
	BOOL m_bReconnect;
	IDBConnection* m_pConnection;
};

class CDBServer : public CIocpTcpServer {
public:
	CDBServer() : CIocpTcpServer(CPS_FLAG_DEFAULT){

	}
	virtual ~CDBServer() {

	}

public:
	virtual void* OnWorkerStart();
	virtual void OnWorkerExit(void* pContext);
	virtual void OnRequest(void* p1, void* p2);

private:
	BOOL DB_TestNormal(CWorkerContext* pContext);
	BOOL DB_TestTrans(CWorkerContext* pContext);
	BOOL DB_TestGetUserInfo(CWorkerContext* pContext, NetPacket* pData);
};
