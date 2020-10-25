#include <iostream>
#include "DBServer.h"

void* CDBServer::OnWorkerStart()
{
	HRESULT hr = ::CoInitialize(NULL);
	if (FAILED(hr))
	{
		myLogConsoleE("%s 初始化COM环境失败", __FUNCTION__);
		return NULL;
	}

	CWorkerContext* pContext = new CWorkerContext;
	if (!pContext)
	{
		myLogConsoleE("%s 空指针异常", __FUNCTION__);
		return NULL;
	}
	pContext->m_pConnection->ConnectDB("47.100.196.145", "root", "8767626LUOLING", "mysql", 3306);
	//DB_TestNormal(pContext);
	//DB_TestTrans(pContext);
	return pContext;
}

void CDBServer::OnWorkerExit(void* pContext)
{
	CWorkerContext* pThreadCxt = (CWorkerContext*)(pContext);
	if (pThreadCxt)
	{
		pThreadCxt->m_pConnection->DisconnDB();
	}
	SAFE_DELETE(pThreadCxt);

	::CoUninitialize();
}

void CDBServer::OnRequest(void* p1, void* p2)
{
	NetPacket* pData = (NetPacket*)p1;
	myLogConsoleI("[%s] recv:[%s]", __FUNCTION__, pData->GetData());

	//std::string str = "hello";
	//SendData(pData->GetCtx(), str.c_str(), str.size());
	//CloseClient(pData->GetCtx()->m_hSocket);

	CWorkerContext* pThreadContext = (CWorkerContext*)p2;
	DB_TestGetUserInfo(pThreadContext, pData);

	if (pThreadContext && pThreadContext->m_bReconnect) {
		pThreadContext->m_pConnection->ReConnect();
	}
}

BOOL CDBServer::DB_TestNormal(CWorkerContext* pContext)
{
	string strSql = "select * from tblUserInfo;";
	pContext->m_pConnection->ExecQuery(strSql);
	while (!pContext->m_pConnection->EndOfFile()) {
		int nId = 0;
		pContext->m_pConnection->GetFieldValue("u_id", nId);
		std::string strName = "";
		pContext->m_pConnection->GetFieldValue("u_name", strName);
		pContext->m_pConnection->QueryNext();
		myLogConsoleI("u_id:%d u_name:%s", nId, strName.c_str());
	}
	pContext->m_pConnection->QueryClose();

	char szSql[1024] = { 0 };
	sprintf_s(szSql, "insert into tblUserInfo(u_id,u_name) values(%lld,'20201025_%lld')", (LONGLONG)time(nullptr), (LONGLONG)time(nullptr));
	strSql = std::string(szSql);
	pContext->m_pConnection->Execute(strSql);
	return TRUE;
}

BOOL CDBServer::DB_TestTrans(CWorkerContext* pContext)
{
	int nTrans = pContext->m_pConnection->BeginTrans();
	if (nTrans > 0) {
		char szSql[1024] = { 0 };
		sprintf_s(szSql, "insert into tblUserInfo(u_id,u_name) values(%lld,'20201025_%lld')", (LONGLONG)time(nullptr), (LONGLONG)time(nullptr));
		std::string strSql = std::string(szSql);
		pContext->m_pConnection->Execute(strSql);
		if (true) {
			pContext->m_pConnection->Commits();
		}
		else {
			pContext->m_pConnection->Rollback();
		}
	}
	return TRUE;
}

BOOL CDBServer::DB_TestGetUserInfo(CWorkerContext* pContext, NetPacket* pData)
{
	char szSql[1024] = { 0 };
	std::string strName = "";
	int nUserId = atoi((const char*)pData->GetData());
	sprintf_s(szSql, "select * from tblUserInfo where u_id=%d;", nUserId);
	pContext->m_pConnection->ExecQuery(std::string(szSql));
	if (!pContext->m_pConnection->EndOfFile()) {
		pContext->m_pConnection->GetFieldValue("u_name", strName);
	}
	pContext->m_pConnection->QueryClose();
	return SendData(pData->GetCtx()->m_hSocket, strName.c_str(), strName.size());
}

int main()
{
	CDBServer* pDBServer = new CDBServer;
	if (!pDBServer || !pDBServer->Initialize("127.0.0.1", 8888, 32, 64, 1, 10000))
	{
		myLogConsoleI("db server initialize failed");
	}
	char g = getchar();
	while ('q' == g)
	{
		pDBServer->Shutdown();
		break;
	}
	return 0;
}
