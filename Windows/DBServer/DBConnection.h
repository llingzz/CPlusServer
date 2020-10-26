#pragma
#include <my_global.h>
#include <errmsg.h>
#include <mysql.h>
#include <vector>
#include <map>
#include "log.h"
#include "IDBConnection.h"

class CDBConnection : public IDBConnection
{
public:
	CDBConnection() {
		m_strDBHost = "";
		m_strUserName = "";
		m_strPassword = "";
		m_strDBName = "";
		m_nPort = 0;
		m_bState = false;
		m_pMysql = NULL;
		m_pMyRes = NULL;
		m_nTotalRows = 0;
		m_nTotalFields = 0;
		m_nCurRowIdx = 0;
	}
	~CDBConnection() {

	}

private:
	CDBConnection(const CDBConnection&) = delete;
	CDBConnection& operator=(const CDBConnection&) = delete;

public:
	int Execute(const string& strSql) {
		ClearRes();
		if (strSql.length() <= 0) {
			myLogConsoleE("%s sql字符串非法:%s", __FUNCTION__, strSql.c_str());
			return 0;
		}
		if (m_pMysql == NULL) {
			if (!ReConnect()) {
				myLogConsoleE("%s 重连DB失败:%s", __FUNCTION__, strSql.c_str());
				return 0;
			}
		}
		int nRet = mysql_real_query(m_pMysql, strSql.c_str(), strSql.length());
		if (nRet != 0) {
			LogDBError();
			myLogConsoleE("%s 执行sql失败:%s", __FUNCTION__, strSql.c_str());
			return 0;
		}
		return (int)(mysql_affected_rows(m_pMysql));
	}
	int ExecQuery(const string& strSql) {
		ClearRes();
		if (strSql.length() < 1) {
			return 0;
		}
		if (m_pMysql == NULL) {
			if (!ReConnect()) {
				myLogConsoleE("%s 重连DB失败:%s", __FUNCTION__, strSql.c_str());
				return 0;
			}
		}
		int nRet = mysql_real_query(m_pMysql, strSql.c_str(), strSql.length());
		if (nRet != 0) {
			LogDBError();
			myLogConsoleE("%s 执行sql失败:%s", __FUNCTION__, strSql.c_str());
			return 0;
		}
		m_pMyRes = mysql_store_result(m_pMysql);
		m_nTotalFields = mysql_num_fields(m_pMyRes);
		m_nTotalRows = static_cast<int>(mysql_affected_rows(m_pMysql));
		GetQueryFields();
		return m_nTotalRows;
	}
	int BeginTrans() {
		std::string strQuery = "START TRANSACTION";
		mysql_real_query(m_pMysql, strQuery.c_str(), (unsigned long)strQuery.size());
		return 1;
	}
	int Commits() {
		std::string strQuery = "COMMIT";
		mysql_real_query(m_pMysql, strQuery.c_str(), (unsigned long)strQuery.size());
		return 1;
	}
	int Rollback() {
		std::string strQuery = "ROLLBACK";
		mysql_real_query(m_pMysql, strQuery.c_str(), (unsigned long)strQuery.size());
		return 1;
	}
	BOOL ConnectDB(const string& strDBHost, const string& strUserName, const string& strPassword, const string& strDBName, const int& nPort) {
		InitMembers(strDBHost, strUserName, strPassword, strDBName, nPort);
		m_pMysql = mysql_init(NULL);
		if (m_pMysql == NULL) {
			LogDBError();
			myLogConsoleE("%s MY_SQL初始化失败,DB连接失败", __FUNCTION__);
			return FALSE;
		}
		char value = 1;
		mysql_options(m_pMysql, MYSQL_OPT_RECONNECT, &value);
		mysql_options(m_pMysql, MYSQL_SET_CHARSET_NAME, "utf8");
		bool bFlag = mysql_real_connect(m_pMysql, m_strDBHost.c_str(), m_strUserName.c_str(), m_strPassword.c_str(), m_strDBName.c_str(), m_nPort, NULL, CLIENT_MULTI_STATEMENTS);
		if (!bFlag) {
			LogDBError();
			myLogConsoleE("%s DB连接失败", __FUNCTION__);
			return FALSE;
		}
		m_bState = true;
		return TRUE;
	}
	BOOL ReConnect() {
		DisconnDB();
		return ConnectDB(m_strDBHost, m_strUserName, m_strPassword, m_strDBName, m_nPort);
	}
	void DisconnDB() {
		m_bState = false;
		if (m_pMysql) {
			mysql_close(m_pMysql);
			m_pMysql = NULL;
		}
		if (m_pMyRes) {
			m_pMyRes = NULL;
		}
	}
	void GetFieldValue(const char* szFieldName, int& n32Data) {
		auto iter = m_mapFieldsValue.find(szFieldName);
		if (iter != m_mapFieldsValue.end()) {
			n32Data = (int)atoi(iter->second.data());
		}
	}
	void GetFieldValue(const char* szFieldName, unsigned int& un32Data) {
		auto iter = m_mapFieldsValue.find(szFieldName);
		if (iter != m_mapFieldsValue.end()) {
			un32Data = (int)atoi(iter->second.data());
		}
	}
	void GetFieldValue(const char* szFieldName, int64_t& n64Data) {
		auto iter = m_mapFieldsValue.find(szFieldName);
		if (iter != m_mapFieldsValue.end()) {
			n64Data = _atoi64(iter->second.data());
		}
	}
	void GetFieldValue(const char* szFieldName, uint64_t& un64Data) {
		auto iter = m_mapFieldsValue.find(szFieldName);
		if (iter != m_mapFieldsValue.end()) {
			un64Data = _atoi64(iter->second.data());
		}
	}
	void GetFieldValue(const char* szFieldName, char* szValue) {
		auto iter = m_mapFieldsValue.find(szFieldName);
		if (iter != m_mapFieldsValue.end()) {
			memcpy(szValue, iter->second.data(), iter->second.length());
		}
	}
	void GetFieldValue(const char* szFieldName, string& strValue) {
		auto iter = m_mapFieldsValue.find(szFieldName);
		if (iter != m_mapFieldsValue.end()) {
			strValue = std::string(iter->second.data());
		}
	}
	void GetQueryFields() {
		m_vectFiled.clear();
		m_mapFieldsValue.clear();
		MYSQL_FIELD* pFiled = NULL;
		MYSQL_ROW curRow = mysql_fetch_row(m_pMyRes);
		if (!curRow) {
			return;
		}
		int i = 0;
		while (pFiled = mysql_fetch_field(m_pMyRes)) {
			auto tempRes = curRow[i];
			if (tempRes) {
				string filedStr(pFiled->name, pFiled->name_length);
				m_mapFieldsValue[filedStr] = tempRes;
				m_vectFiled.push_back(filedStr);
			}
			++i;
		}
	}
	void QueryNext() {
		m_mapFieldsValue.clear();
		MYSQL_ROW curRow = mysql_fetch_row(m_pMyRes);
		if (!curRow) {
			return;
		}
		m_nCurRowIdx++;
		int size = m_vectFiled.size();
		for (int i = 0; i < size; ++i) {
			auto tempRes = curRow[i];
			if (tempRes) {
				m_mapFieldsValue[m_vectFiled[i]] = tempRes;
			}
		}
	}
	void QueryClose() {
		m_nCurRowIdx = 0;
		m_nTotalRows = 0;
		m_mapFieldsValue.clear();
	}
	bool EndOfFile() {
		return (m_nTotalRows <= 0) || ((m_nCurRowIdx + 1) > m_nTotalRows);
	}

private:
	void LogDBError() {
		m_uError = mysql_errno(m_pMysql);
		m_strError = std::string(mysql_error(m_pMysql));
		myLogConsoleE("errorCode:%u errorStr:%s", m_uError, m_strError.c_str());
	}
	void InitMembers(const string& strDBHost, const string& strUserName, const string& strPassword, const string& strDBName, const int& nPort) {
		m_strDBHost = strDBHost;
		m_strUserName = strUserName;
		m_strPassword = strPassword;
		m_strDBName = strDBName;
		m_nPort = nPort;
	}
	void ClearRes() {
		bool ifNeedQueryNext = (m_pMyRes != nullptr);
		if (m_pMyRes) {
			mysql_free_result(m_pMyRes);
		}
		if (ifNeedQueryNext) {
			while (mysql_next_result(m_pMysql) == 0) {
				m_pMyRes = mysql_store_result(m_pMysql);
				if (m_pMyRes) {
					mysql_free_result(m_pMyRes);
				}
			}
		}
		else {
			do {
				m_pMyRes = mysql_store_result(m_pMysql);
				if (m_pMyRes) {
					mysql_free_result(m_pMyRes);
				}
			} while (mysql_next_result(m_pMysql) == 0);
		}
		m_pMyRes = NULL;
	}

private:
	MYSQL*									m_pMysql;
	MYSQL_RES*								m_pMyRes;

	int										m_nPort;
	int										m_nTotalRows;
	int										m_nTotalFields;
	int										m_nCurRowIdx;

	bool									m_bState;
	unsigned int							m_uError;

	std::string								m_strError;
	std::string								m_strDBHost;
	std::string								m_strUserName;
	std::string								m_strPassword;
	std::string								m_strDBName;
	std::vector<std::string>				m_vectFiled;
	std::map<std::string, std::string>		m_mapFieldsValue;
};
