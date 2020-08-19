#pragma once

#include "iocp.h"
#include "DBConnection.h"

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

	void ConnectDB(CWorkerContext* pContext);
	std::string GetDBConnectionStr();

private:
	BOOL DB_TestNormal(CWorkerContext* pContext);
	BOOL DB_TestTrans(CWorkerContext* pContext);
};
