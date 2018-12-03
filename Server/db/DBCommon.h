#pragma once

#import "dll/msado15.dll" no_namespace rename("EOF","adoEOF") rename("BOF","adoBOF")

#define	MAX_SQL_LENGTH	1024	//SQL语句的最大长度

//经常使用,内联关键词修饰
inline void TESTHR(HRESULT hr)	
{
	if (FAILED(hr))
	{
		_com_issue_error(hr);
	}
};

static int MYSQL_BeginTrans(_ConnectionPtr& connection)
{
	int nResult = 0;
	try{
		TESTHR(connection->BeginTrans());
		nResult = 1;
	}
	catch (_com_error e)
	{
		nResult = -1;
		printf(e.Description());
	}
	return nResult;
}
static int MYSQL_CommitTrans(_ConnectionPtr& connection)
{
	int nResult = 0;
	try{
		TESTHR(connection->CommitTrans());
		nResult = 1;
	}
	catch (_com_error e)
	{
		nResult = -1;
		printf(e.Description());
	}
	return nResult;
}
static int MYSQL_RollbackTrans(_ConnectionPtr& connection)
{
	int nResult = 0;
	try{
		TESTHR(connection->RollbackTrans());
		nResult = 1;
	}
	catch (_com_error e)
	{
		nResult = -1;
		printf(e.Description());
	}
	return nResult;
}

static int MYSQL_TestConnection(_ConnectionPtr& connection)
{
	int nResult = 0;
	char szSQL[MAX_SQL_LENGTH];
	sprintf_s(szSQL, "select * from test");
	try{
		_RecordsetPtr pRecordset = NULL;
		TESTHR(pRecordset.CreateInstance(__uuidof(Recordset)));
		TESTHR(pRecordset->Open(szSQL, _variant_t((IDispatch*)connection, true), adOpenDynamic, adLockOptimistic, adCmdText));
		if (FALSE == pRecordset->adoEOF)
		{
			printf("connect successful...\n");
			nResult = 1;
		}
		TESTHR(pRecordset->Close());
	}
	catch (_com_error e)
	{
		nResult = -1;
		printf(e.Description());
	}
	return nResult;
}