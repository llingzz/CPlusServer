#pragma once

#include "include/mysql_api/mysql.h"
#include "include/mysql_api/errmsg.h"

class CMysqlRecordSet
{
public:
	CMysqlRecordSet();
	virtual ~CMysqlRecordSet();

	void SetFieldType(int nIndex, enum_field_types enType);
	bool InitRecordSet(MYSQL_STMT* pMySqlStmt, MYSQL_RES* pResult);
	bool ClearRecordSet();

	bool GetBool(int nIndex);
	bool GetBool(std::string strField);

	int GetInt(int nIndex);
	int GetInt(std::string strField);

	float GetFloat(int nIndex);
	float GetFloat(std::string strField);

	double GetDouble(int nIndex);
	double GetDouble(std::string strField);

	std::string GetString(int nIndex);
	std::string GetString(std::string strField);

public:
	MYSQL_BIND*    m_pBinds;

	int            m_nRowCount;
	int            m_nFieldNum;

	std::map<std::string, int> m_FieldIndex;
};

class CMysqlConnection
{
public:
	CMysqlConnection();
	virtual ~CMysqlConnection();

	bool Init();
	void UnInit();

	bool Connect(std::string strHost, std::string strUser, std::string strPwd, std::string strDB, int nPort, std::string strCharset = "utf8");
	bool Reconnect();
	void Close();

	int Excute(std::string strSql, CMysqlRecordSet& recordSet);
	void Query(std::string strSql, CMysqlRecordSet& recordSet);

private:
	MYSQL*         m_pMySql;

	std::string    m_strHost;
	std::string    m_strUser;
	std::string    m_strPwd;
	std::string    m_strDB;
	std::string    m_strCharSet;

	int            m_nPort;
};