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

	TCHAR szFilePath[MAX_PATH];
	GetModuleFileName(GetModuleHandle(NULL), szFilePath, MAX_PATH - 1);
	std::string strFilePath = TCHAR2CHAR(szFilePath);
	const char* ptr = strrchr(strFilePath.c_str(), '\\');
	size_t nIndex = strFilePath.find(ptr);
	string strRes = strFilePath.substr(0, nIndex);
	strRes.append(std::string("\\DBServer.ini"));

	TCHAR wszFile[MAX_PATH];
	CHAR2TCHAR(wszFile, strRes.c_str());

	TCHAR wszIp[24] = { 0 };
	GetPrivateProfileString(_T("DB"), _T("ip"), _T(""), wszIp, 24, wszFile);
	int nPort = 3306;
	nPort = GetPrivateProfileInt(_T("DB"), _T("port"), 3306, wszFile);
	TCHAR wszUsr[24] = { 0 };
	GetPrivateProfileString(_T("DB"), _T("usr"), _T("root"), wszUsr, 24, wszFile);
	TCHAR wszPwd[24] = { 0 };
	GetPrivateProfileString(_T("DB"), _T("pwd"), _T(""), wszPwd, 24, wszFile);
	TCHAR wszDB[24] = { 0 };
	GetPrivateProfileString(_T("DB"), _T("db"), _T(""), wszDB, 24, wszFile);

	std::string strIp = TCHAR2CHAR(wszIp);
	std::string strUsr = TCHAR2CHAR(wszUsr);
	std::string strPwd = TCHAR2CHAR(wszPwd);
	std::string strDB = TCHAR2CHAR(wszDB);

	pContext->m_pConnection->ConnectDB(strIp, strUsr, strPwd, strDB, nPort);

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
	CSocketContext* pContext = (CSocketContext*)p1;
	NetRequest* pRequest = (NetRequest*)p2;
	CWorkerContext* pThreadContext = (CWorkerContext*)GetWorkerContext();

	__try {
		switch (pRequest->head.nRequest) {
		case emOnGetUserInfo:
			OnGetUserInfo(pContext, pRequest, pThreadContext);
			break;
		default:
			OnUnsupported(pContext, pRequest, pThreadContext);
			break;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {

	}

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

BOOL CDBServer::OnUnsupported(CSocketContext* pContext, NetRequest* pRequest, CWorkerContext* pThreadCtx)
{
	return TRUE;
}

BOOL CDBServer::OnGetUserInfo(CSocketContext* pContext, NetRequest* pRequest, CWorkerContext* pThreadCtx)
{
	ContextHead* pContextHead = (ContextHead*)((PBYTE)pRequest->pData);
	int* pUserId = (int*)((PBYTE)pRequest->pData + pRequest->head.nRepeated * sizeof(ContextHead));
	if (pUserId) {
		int nUserId = *pUserId;
		std::string strRet = "";
		char szSql[1024] = { 0 };
		try {
			sprintf_s(szSql, "select * from tblUserInfo where u_id=%d;", nUserId);
			pThreadCtx->m_pConnection->ExecQuery(std::string(szSql));
			if (!pThreadCtx->m_pConnection->EndOfFile()) {
				pThreadCtx->m_pConnection->GetFieldValue("u_name", strRet);
			}
			pThreadCtx->m_pConnection->QueryClose();
		}
		catch (...) {
			myLogConsoleE("%s catch error! sql:%s", __FUNCTION__, szSql);
		}

		NetPacketHead stHead = { 0 };
		stHead.nDataLen = sizeof(NetRequest) + sizeof(ContextHead) + strRet.size();
		NetRequest req;
		req.head.nRepeated = 1;
		ContextHead head;
		head.hSocket = pContextHead->hSocket;
		CBuffer objBuffer;
		objBuffer.Write((PBYTE)&stHead, sizeof(NetPacketHead));
		objBuffer.Write((PBYTE)&req, sizeof(NetRequest));
		objBuffer.Write((PBYTE)&head, sizeof(ContextHead));
		objBuffer.Write((PBYTE)strRet.c_str(), strRet.size());
		SendData(pContext->m_hSocket, objBuffer.GetBuffer(), objBuffer.GetBufferLen());
	}
	return TRUE;
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
