#pragma
#include <winsock2.h>
#include <tchar.h>
#include <my_global.h>
#include <errmsg.h>
#include <mysql.h>
#include <iostream>
#include <string>
#include <map>
#include <vector>

#pragma comment(lib, "libmysql.lib")
//run OleView.exe from the Visual Studio Command Prompt. File + View Typelib and navigate to msado15.dll.
//You'll see the guid you need right at the top, the uuid() attribute on the library section
//#import "libid:B691E011-1797-432E-907A-4D8C69339129" no_namespace rename("EOF","adoEOF") rename("BOF","adoBOF")
#import "C:/Program Files/Common Files/System/ado/msado15.dll" no_namespace rename("EOF","adoEOF") rename("BOF","adoBOF")

extern BOOL DB_SafeOpen(const std::string& strConnection, _ConnectionPtr& pConn);

extern BOOL DB_SafeClose(_ConnectionPtr& pConn);

extern UINT DB_BeginTrans(_ConnectionPtr& pConn, LONG& errcode, UINT& response);

extern UINT DB_Commit(_ConnectionPtr& pConn, LONG& errcode, UINT& response);

extern UINT DB_Rollback(_ConnectionPtr& pConn, LONG& errcode, UINT& response);

class CMysqlField {
public:
	CMysqlField() {
		ulLenght = 0;
		strValue = "";
		strFieldName = "";
		enTypes = MYSQL_TYPE_LONG;
	}
	virtual ~CMysqlField() {
		
	}
	int asInt() {
		return atoi(strValue.c_str());
	}
	long asLong() {
		return atol(strValue.c_str());
	}
	float asFloat() {
		return atof(strValue.c_str());
	}
	long long asLonglong() {
		return atoll(strValue.c_str());
	}
	std::string asString() {
		return strValue;
	}
public:
	enum_field_types enTypes;
	std::string strFieldName;
	std::string strValue;
	unsigned long ulLenght;
};

class CMysqlRow {
public:
	CMysqlRow() {

	}
	virtual ~CMysqlRow() {

	}
public:
	std::map<std::string, CMysqlField> fieldMap;
};

class CMysqlRecordset {
public:
	CMysqlRecordset() {

	}
	virtual ~CMysqlRecordset() {

	}
	bool IsEndOfFile() {
		return iter == rowMap.end();
	}
	void MoveNext() {
		iter++;
	}
	CMysqlField GetCollect(std::string strField) {
		return iter->second.fieldMap[strField];
	}
public:
	std::map<int, CMysqlRow>::iterator iter;
	std::map<int, CMysqlRow> rowMap;
};

class CDBConnection {
public:
	CDBConnection();
	virtual ~CDBConnection();

public:
	BOOL Init();
	BOOL Uninit();
	BOOL SetConnectParam(char const* szHost, char const* szUser, char const* szPwd, char const* szDb, int nPort, char const* szCharSet = "utf8");
	BOOL Connect(char const* szHost, char const* szUser, char const* szPwd, char const* szDb, int nPort, char const* szCharSet = "utf8");
	BOOL Connect();
	BOOL Reconnect();
	BOOL Ping();
	BOOL Close();
	BOOL BeginTrans();
	BOOL Rollback();
	BOOL Commits();
	UINT GetError();
	void Query(std::string strSql, CMysqlRecordset& recordSet);
	void Execute(std::string strSql, int& nAffectRows);
	void StmtExecute(std::string strSql, int& nAffectRows);

private:
	MYSQL* m_pMySql;
	INT m_nPort;
	UINT m_nErrno;
	std::string m_strHost;
	std::string m_strUser;
	std::string m_strPwd;
	std::string m_strDB;
	std::string m_strError;
	std::string m_strCharSet;
};

class CWorkerContext {
public:
	CWorkerContext() {
		m_bReconnectMain = FALSE;
		m_pConnectionMain = NULL;
		m_pConnection = new CDBConnection;
	}
	virtual ~CWorkerContext() {

	}

public:
	BOOL m_bReconnectMain;
	_ConnectionPtr m_pConnectionMain;
	CDBConnection* m_pConnection;
};
