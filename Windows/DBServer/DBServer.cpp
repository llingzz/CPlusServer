#include <iostream>
#include "DBServer.h"

void CDBServer::ConnectDB(CWorkerContext* pContext)
{
	//DB_SafeClose(pContext->m_pConnectionMain);
	//DB_SafeOpen(GetDBConnectionStr(), pContext->m_pConnectionMain);
	pContext->m_pConnection->Init();
	pContext->m_pConnection->Connect("47.100.196.145", "root", "8767626LUOLING", "mysql", 3306);
}

std::string CDBServer::GetDBConnectionStr()
{
	return std::string("DRIVER={MySQL ODBC 8.0 Unicode Driver};Server=47.100.196.145;Database=mysql;UID=root;PWD=8767626LUOLING;");
}

void* CDBServer::OnWorkerStart()
{
	HRESULT hr = ::CoInitialize(NULL);
	if (FAILED(hr)) {
		return NULL;
	}
	CWorkerContext* pContext = new CWorkerContext;
	ConnectDB(pContext);

	CMysqlRecordset recordSet;
	pContext->m_pConnection->Query("select * from tblUserInfo;", recordSet);
	while (!recordSet.IsEndOfFile())
	{
		auto v1 = recordSet.GetCollect("u_id").asInt();
		auto v2 = recordSet.GetCollect("u_name").asString();
		recordSet.MoveNext();
		myLogConsoleI("result:u_id:%d u_name:%s", v1, v2.c_str());
	}

	int nAffectRows = 0;
	char szSql[1024] = { 0 };
	time_t tNow = time(nullptr);
	sprintf_s(szSql, "insert into tblUserInfo(u_id, u_name) values(%d,'luoling_%d');", (int)tNow, (int)tNow);
	pContext->m_pConnection->Execute(szSql, nAffectRows);
	if (nAffectRows <= 0)
	{
		myLogConsoleE("%s %s executed failed...", __FUNCTION__, szSql);
	}

	return pContext;
}

void CDBServer::OnWorkerExit(void* pContext)
{
	CWorkerContext* pThreadCxt = (CWorkerContext*)(pContext);
	if (pThreadCxt)
	{
		//DB_SafeClose(pThreadCxt->m_pConnectionMain);
	}
	SAFE_DELETE(pThreadCxt);
	::CoUninitialize();
}

void CDBServer::OnRequest(void* p1, void* p2)
{
	NetPacket* pData = (NetPacket*)p1;
	CWorkerContext* pThreadContext = (CWorkerContext*)p2;

#if 0
	//DB_TestNormal(pThreadContext);
#else
	//DB_TestTrans(pThreadContext);
#endif

	myLogConsoleI("[%s] recv:[%s]", __FUNCTION__, pData->GetData());

	std::string str = "hello";
	SendData(pData->GetCtx(), str.c_str(), str.size());
	//CloseClient(pData->GetCtx());
	//CloseClient(pData->GetCtx()->m_hSocket);

	/*if (pThreadContext->m_bReconnectMain) {
		ConnectDB(pThreadContext);
	}*/
}

BOOL CDBServer::DB_TestNormal(CWorkerContext* pContext)
{
	_ConnectionPtr pConn = pContext->m_pConnectionMain;
	_variant_t RecordsAffected;
	TCHAR szSql[1024];
	try
	{
#if 1
		swprintf_s(szSql, _T("insert into tblUserInfo(u_id, u_name) values(2, '2');"));
		auto res = pConn->Execute(szSql, &RecordsAffected, adCmdText);
		if (RecordsAffected.lVal > 0)
		{
			myLogConsoleI("insert into tblUserInfo(u_id, u_name) values(2, '2'); success...");
		}
#elif 0
		_stprintf(szSql, _T("select * from tblUserInfo;"));
		_RecordsetPtr pRecordset;
		pRecordset.CreateInstance("ADODB.Recordset");
		pRecordset = pConn->Execute(szSql, &RecordsAffected, adCmdText);
		while (!pRecordset->adoEOF)
		{
			_variant_t vTemp1;
			vTemp1 = pRecordset->GetCollect("u_id");
			_variant_t vTemp2;
			vTemp2 = pRecordset->GetCollect("u_name");
			myLogConsoleI("get u_id:%d u_name:%s", vTemp1.intVal, _com_util::ConvertBSTRToString(vTemp2.bstrVal));
			pRecordset->MoveNext();
		}
		pRecordset->Close();
#endif
	}
	catch (_com_error e)
	{
		auto pError = pConn->Errors;
		auto lCount = pError->Count;
		for (long i = 0; i < lCount; i++)
		{
			ErrorPtr pErr = pError->GetItem(i);
			std::string strError = std::string(pErr->SQLState);
			if ("01002" == strError || // 断开连接错误
				"08S0" == strError || // 连接失败
				"08S01" == strError || // 通讯链接失败
				"08001" == strError || // 无法连接到数据源
				"08004" == strError || // 数据源拒绝建立连接
				"08007" == strError)   // 在执行事务的过程中连接失败
			{
				pContext->m_bReconnectMain = TRUE;
			}
		}
		return FALSE;
	};
	return TRUE;
}

BOOL CDBServer::DB_TestTrans(CWorkerContext* pContext)
{
	_ConnectionPtr pConn = pContext->m_pConnectionMain;
	_variant_t RecordsAffected;
	TCHAR szSql[1024];

	LONG lError = 0;
	UINT nResponse = 0;
	UINT nTrans = DB_BeginTrans(pConn, lError, nResponse);
	if (nTrans <= 0)
	{
		return FALSE;
	}

	BOOL bResult = TRUE;
	try {
		swprintf_s(szSql, _T("insert into tblUserInfo(u_id, u_name) values(2, '2');"));
		auto res = pConn->Execute(szSql, &RecordsAffected, adCmdText);
		if (RecordsAffected.lVal > 0)
		{
			myLogConsoleI("insert into tblUserInfo(u_id, u_name) values(2, '2'); success...");
		}
	}
	catch (_com_error e)
	{
		bResult = FALSE;
	};

	try {
		swprintf_s(szSql, _T("insert into tblUserInfo(u_id, u_name) values(3, '3');"));
		auto res = pConn->Execute(szSql, &RecordsAffected, adCmdText);
		if (RecordsAffected.lVal > 0)
		{
			myLogConsoleI("insert into tblUserInfo(u_id, u_name) values(3, '3'); success...");
		}
	}
	catch (_com_error e)
	{
		bResult = FALSE;
	};

	if (bResult)
	{
		DB_Commit(pConn, lError, nResponse);
		myLogConsoleI("DB_Commit...");
	}
	else
	{
		DB_Rollback(pConn, lError, nResponse);
		myLogConsoleI("DB_Rollback...");
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
