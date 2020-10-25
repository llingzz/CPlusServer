#pragma once
#include <iostream>
#include <string>
#include <windows.h>
using namespace std;
class IDBConnection
{
public:
	virtual int Execute(const string& strSql) = 0;
	virtual int ExecQuery(const string& strSql) = 0;
	virtual int BeginTrans() = 0;
	virtual int Commits() = 0;
	virtual int Rollback() = 0;
	virtual BOOL ConnectDB(const string& strDBHost, const string& strUserName, const string& strPassword, const string& strDBName, const int& nPort) = 0;
	virtual BOOL ReConnect() = 0;
	virtual void DisconnDB() = 0;
	virtual void GetFieldValue(const char* szFieldName, int& n32Data) = 0;
	virtual void GetFieldValue(const char* szFieldName, unsigned int& un32Data) = 0;
	virtual void GetFieldValue(const char* szFieldName, int64_t& n64Data) = 0;
	virtual void GetFieldValue(const char* szFieldName, uint64_t& un64Data) = 0;
	virtual void GetFieldValue(const char* szFieldName, char* szValue) = 0;
	virtual void GetFieldValue(const char* szFieldName, string& strValue) = 0;
	virtual void GetQueryFields() = 0;
	virtual void QueryNext() = 0;
	virtual void QueryClose() = 0;
	virtual bool EndOfFile() = 0;
};
