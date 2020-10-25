#pragma once
#include <tchar.h>
#include "IDBConnection.h"
#pragma comment(lib, "libmysql.lib")
// run OleView.exe from the Visual Studio Command Prompt. File + View Typelib and navigate to msado15.dll.
// You'll see the guid you need right at the top, the uuid() attribute on the library section
//#import "libid:B691E011-1797-432E-907A-4D8C69339129" no_namespace rename("EOF","adoEOF") rename("BOF","adoBOF")
#import "C:/Program Files/Common Files/System/ado/msado15.dll" no_namespace rename("EOF","adoEOF") rename("BOF","adoBOF")

/*ODBC数据库操作*/
class COleDBConnection : public IDBConnection
{
public:
	COleDBConnection(){
		m_strDBHost = "";
		m_strUserName = "";
		m_strPassword = "";
		m_strDBName = "";
		m_nPort = 0;
		m_strConnection = "";
		m_pConnection = NULL;
		m_pRecord = NULL;
	}
	~COleDBConnection() {

	}

private:
	COleDBConnection(const COleDBConnection&) = delete;
	COleDBConnection& operator=(const COleDBConnection&) = delete;

public:
	int Execute(const string& strSql) {
		ClearRes();
		_variant_t RecordsAffected;
		try {
			m_pConnection->Execute((_bstr_t)strSql.c_str(), &RecordsAffected, adCmdText);
		}
		catch (_com_error e) {
			LogDBError(e);
		}
		return RecordsAffected.intVal;
	}
	int ExecQuery(const string& strSql) {
		ClearRes();
		_variant_t RecordsAffected;
		try {
			m_pRecord.CreateInstance("ADODB.Recordset");
			m_pRecord = m_pConnection->Execute((_bstr_t)strSql.c_str(), &RecordsAffected, adCmdText);
		}
		catch (_com_error e) {
			LogDBError(e);
		}
		return RecordsAffected.intVal;
	}
	int BeginTrans() {
		try {
			HRESULT hr = m_pConnection->BeginTrans();
			if FAILED(hr) {
				_com_issue_error(hr);
			}
			return 1;
		}
		catch (_com_error e) {
			LogDBError(e);
		}
		return 0;
	}
	int Commits() {
		try {
			HRESULT hr = m_pConnection->CommitTrans();
			if FAILED(hr) {
				_com_issue_error(hr);
			}
			return 1;
		}
		catch (_com_error e) {
			LogDBError(e);
		}
		return 0;
	}
	int Rollback() {
		try {
			HRESULT hr = m_pConnection->RollbackTrans();
			if FAILED(hr) {
				_com_issue_error(hr);
			}
			return 1;
		}
		catch (_com_error e) {
			LogDBError(e);
		}
		return 0;
	}
	BOOL ConnectDB(const string& strDBHost, const string& strUserName, const string& strPassword, const string& strDBName, const int& nPort) {
		InitMembers(strDBHost, strUserName, strPassword, strDBName, nPort);
		return ConnectDB();
	}
	BOOL ReConnect() {
		return ConnectDB();
	}
	void DisconnDB() {
		try {
			if (m_pConnection) {
				m_pConnection->Close();
				m_pConnection = NULL;
			}
		}
		catch (_com_error e) {
			LogDBError(e);
		}
	}
	void GetFieldValue(const char* szFieldName, int& n32Data) {
		n32Data = m_pRecord->GetCollect(szFieldName).intVal;
	}
	void GetFieldValue(const char* szFieldName, unsigned int& un32Data) {
		un32Data = m_pRecord->GetCollect(szFieldName).uintVal;
	}
	void GetFieldValue(const char* szFieldName, int64_t& n64Data) {
		n64Data = m_pRecord->GetCollect(szFieldName).llVal;
	}
	void GetFieldValue(const char* szFieldName, uint64_t& un64Data) {
		un64Data = m_pRecord->GetCollect(szFieldName).ullVal;
	}
	void GetFieldValue(const char* szFieldName, char* szValue) {
		_variant_t vTemp = m_pRecord->GetCollect(szFieldName);
		if (vTemp.bstrVal != NULL) {
			strcpy(szValue, _com_util::ConvertBSTRToString(vTemp.bstrVal));
		}
	}
	void GetFieldValue(const char* szFieldName, string& strValue) {
		_variant_t vTemp = m_pRecord->GetCollect(szFieldName);
		if (vTemp.bstrVal != NULL) {
			strValue = std::string(_com_util::ConvertBSTRToString(vTemp.bstrVal));
		}
	}
	void GetQueryFields() {
		return;
	}
	void QueryNext() {
		m_pRecord->MoveNext();
	}
	void QueryClose() {
		m_pRecord->Close();
	}
	bool EndOfFile() {
		return m_pRecord->adoEOF;
	}

private:
	void LogDBError(_com_error e) {
		// strError:01002 断开连接错误
		// strError:08S0  连接失败
		// strError:08S01 通讯链接失败
		// strError:08001 无法连接到数据源
		// strError:08004 数据源拒绝建立连接
		// strError:08007 在执行事务的过程中连接失败
		auto pError = m_pConnection->Errors;
		auto lCount = pError->Count;
		for (long i = 0; i < lCount; i++) {
			ErrorPtr pErr = pError->GetItem(i);
			std::string strError = std::string(pErr->SQLState);
			myLogConsoleE("%s 捕获异常:%s", __FUNCTION__, strError.c_str());
		}
	}
	void ClearRes() {
		m_pRecord = NULL;
	}
	void InitMembers(const string& strDBHost, const string& strUserName, const string& strPassword, const string& strDBName, const int& nPort) {
		m_strDBHost = strDBHost;
		m_strUserName = strUserName;
		m_strPassword = strPassword;
		m_strDBName = strDBName;
		m_nPort = nPort;
		SetConnectionString(strDBHost, strUserName, strPassword, strDBName, nPort);
	}
	void SetConnectionString(const string& strDBHost, const string& strUserName, const string& strPassword, const string& strDBName, const int& nPort) {
		char szConnection[1024];
		memset(szConnection, 0, 1024);
		sprintf_s(szConnection, "DRIVER={MySQL ODBC 8.0 Unicode Driver};Server=%s;Database=%s;UID=%s;PWD=%s;", strDBHost.c_str(), strDBName.c_str(), strUserName.c_str(), strPassword.c_str());
		m_strConnection = std::string(szConnection);
	}
	BOOL ConnectDB() {
		try {
			if (m_pConnection) {
				m_pConnection->Close();
				m_pConnection = NULL;
			}
			HRESULT hr = m_pConnection.CreateInstance(__uuidof(Connection));
			if (FAILED(hr)) {
				myLogConsoleE("%s 创建ole连接失败", __FUNCTION__);
				return FALSE;
			}
			m_pConnection->Open(m_strConnection.c_str(), _T(""), _T(""), adConnectUnspecified);
			return TRUE;
		}
		catch (_com_error e) {
			LogDBError(e);
		}
		return FALSE;
	}

private:
	int						m_nPort;
	string					m_strDBHost;
	string					m_strUserName;
	string					m_strPassword;
	string					m_strDBName;
	string					m_strConnection;
	_ConnectionPtr			m_pConnection;
	_RecordsetPtr			m_pRecord;
};
