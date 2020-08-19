#include "DBConnection.h"

extern BOOL DB_SafeOpen(const std::string& strConnection, _ConnectionPtr& pConn) {
	HRESULT hr = pConn.CreateInstance(__uuidof(Connection));
	if (FAILED(hr)) {
		return 0;
	}
	try {
		hr = 1;
		pConn->Open(strConnection.c_str(), _T(""), _T(""), adConnectUnspecified);
	}
	catch (_com_error e) {
		hr = 0;
	}
	return (hr != 0);
}

extern BOOL DB_SafeClose(_ConnectionPtr& pConn) {
	try {
		if (pConn) {
			pConn->Close();
			pConn = NULL;
		}
	}
	catch (_com_error e) {
		return FALSE;
	}
	return TRUE;
}

extern UINT DB_BeginTrans(_ConnectionPtr& pConn, LONG& errcode, UINT& response) {
	UINT nResult = 0;
	try {
		HRESULT hr = pConn->BeginTrans();
		if FAILED(hr) {
			_com_issue_error(hr);
		}
		nResult = 1;
	}
	catch (_com_error e) {

	}
	return nResult;
}

extern UINT DB_Commit(_ConnectionPtr& pConn, LONG& errcode, UINT& response) {
	UINT nResult = 0;
	try {
		HRESULT hr = pConn->CommitTrans();
		if FAILED(hr) {
			_com_issue_error(hr);
		}
		nResult = 1;
	}
	catch (_com_error e) {

	}
	return nResult;
}

extern UINT DB_Rollback(_ConnectionPtr& pConn, LONG& errcode, UINT& response) {
	UINT nResult = 0;
	try {
		HRESULT hr = pConn->RollbackTrans();
		if FAILED(hr) {
			_com_issue_error(hr);
		}
		nResult = 1;
	}
	catch (_com_error e) {

	}
	return nResult;
}

CDBConnection::CDBConnection()
	: m_pMySql(NULL), m_nPort(3306), m_nErrno(0), m_strError("")
{
	
}

CDBConnection::~CDBConnection() {
	Close();
}

BOOL CDBConnection::Init()
{
	mysql_library_init(0, NULL, NULL);
	mysql_thread_init();
	return TRUE;
}

BOOL CDBConnection::Uninit() {
	mysql_thread_end();
	mysql_library_end();
	return TRUE;
}

BOOL CDBConnection::SetConnectParam(char const* szHost, char const* szUser, char const* szPwd, char const* szDb, int nPort, char const* szCharSet) {
	m_strHost.assign(szHost);
	m_strUser.assign(szUser);
	m_strPwd.assign(szPwd);
	m_strDB.assign(szDb);
	m_nPort = nPort;
	m_strCharSet.assign(szCharSet);
	return TRUE;
}

BOOL CDBConnection::Connect(char const* szHost, char const* szUser, char const* szPwd, char const* szDb, int nPort, char const* szCharSet) {
	Close();
	SetConnectParam(szHost, szUser, szPwd, szDb, nPort, szCharSet);
	return Connect();
}

BOOL CDBConnection::Connect() {
	Close();
	m_pMySql = mysql_init(NULL);
	if (m_pMySql == NULL) {
		return FALSE;
	}
	if (0 != mysql_options(m_pMySql, MYSQL_SET_CHARSET_NAME, m_strCharSet.c_str())) {
		mysql_close(m_pMySql);
		m_pMySql = NULL;
		return FALSE;
	}
	if (NULL == mysql_real_connect(m_pMySql, m_strHost.c_str(), m_strUser.c_str(), m_strPwd.c_str(), m_strDB.c_str(), m_nPort, NULL, 0)) {
		m_nErrno = mysql_errno(m_pMySql);
		m_strError = mysql_error(m_pMySql);
		mysql_close(m_pMySql);
		m_pMySql = NULL;
		return FALSE;
	}
	return TRUE;
}

BOOL CDBConnection::Ping() {
	if (mysql_ping(m_pMySql) == 0) {
		return TRUE;
	}
	return FALSE;
}

BOOL CDBConnection::Reconnect() {
	if (Ping()) {
		return TRUE;
	}
	return Connect();
}

UINT CDBConnection::GetError() {
	return m_nErrno;
}

BOOL CDBConnection::Close() {
	if (m_pMySql) {
		mysql_close(m_pMySql);
		m_pMySql = NULL;
	}
	return TRUE;
}

BOOL CDBConnection::BeginTrans() {
	std::string strQuery = "START TRANSACTION";
	if (!mysql_real_query(m_pMySql, strQuery.c_str(), (unsigned long)strQuery.size())) {
		return FALSE;
	}
	return TRUE;
}

BOOL CDBConnection::Rollback() {
	std::string strQuery = "ROLLBACK";
	if (!mysql_real_query(m_pMySql, strQuery.c_str(), (unsigned long)strQuery.size())) {
		return FALSE;
	}
	return TRUE;
}

BOOL CDBConnection::Commits() {
	std::string strQuery = "COMMIT";
	if (!mysql_real_query(m_pMySql, strQuery.c_str(), (unsigned long)strQuery.size())) {
		return FALSE;
	}
	return TRUE;
}

void CDBConnection::Query(std::string strSql, CMysqlRecordset& recordSet)
{
	int nRet = mysql_real_query(m_pMySql, strSql.c_str(), strSql.size());
	if (nRet)
	{
		UINT nErrno = mysql_errno(m_pMySql);
		if (CR_SERVER_GONE_ERROR == nErrno)
		{
			Reconnect();
			nRet = mysql_real_query(m_pMySql, strSql.c_str(), strSql.size());
			if (nRet)
			{
				return;
			}
		}
		else
		{
			return;
		}
	}
	MYSQL_RES* pResult = mysql_store_result(m_pMySql);

	int nFieldNums = 0;
	MYSQL_FIELD* pFields;
	std::map<int, CMysqlField> map;
	while (NULL != (pFields = mysql_fetch_field(pResult)))
	{
		map[nFieldNums].enTypes = pFields->type;
		map[nFieldNums].strFieldName = pFields->name;
		nFieldNums++;
	}

	int nIndex = 0;
	unsigned long* lLengths;
	MYSQL_ROW row;
	while (NULL != (row = mysql_fetch_row(pResult)))
	{
		CMysqlRow objRow;
		lLengths = mysql_fetch_lengths(pResult);
		for (auto i = 0; i < nFieldNums; i++)
		{
			CMysqlField& field = map[i];
			CMysqlField newField;
			newField.enTypes = field.enTypes;
			newField.strFieldName = field.strFieldName;
			newField.strValue = row[i];
			newField.ulLenght = lLengths[i];
			objRow.fieldMap[field.strFieldName] = std::move(newField);
		}
		recordSet.rowMap[nIndex++] = std::move(objRow);
	}
	recordSet.iter = recordSet.rowMap.begin();
	mysql_free_result(pResult);
}

void CDBConnection::Execute(std::string strSql, int& nAffectRows)
{
	int nRet = mysql_real_query(m_pMySql, strSql.c_str(), strSql.size());
	if (nRet)
	{
		UINT nErrno = mysql_errno(m_pMySql);
		if (CR_SERVER_GONE_ERROR == nErrno)
		{
			Reconnect();
			nRet = mysql_real_query(m_pMySql, strSql.c_str(), strSql.size());
			if (nRet)
			{
				return;
			}
		}
		else
		{
			return;
		}
	}
	nAffectRows = mysql_affected_rows(m_pMySql);
}

void CDBConnection::StmtExecute(std::string strSql, int& nAffectRows)
{
	MYSQL_STMT* pMySqlStmt = mysql_stmt_init(m_pMySql);
	mysql_stmt_prepare(pMySqlStmt, strSql.c_str(), (ULONG)strSql.size());
	mysql_stmt_param_count(pMySqlStmt);

	MYSQL_RES* pResult = mysql_stmt_result_metadata(pMySqlStmt);
	mysql_stmt_execute(pMySqlStmt);
	int nFieldCount = pResult->field_count;
	MYSQL_BIND* pResBind = new MYSQL_BIND[nFieldCount];
	memset(pResBind, 0, nFieldCount * sizeof(MYSQL_BIND));

	mysql_stmt_bind_result(pMySqlStmt, pResBind);
	mysql_stmt_store_result(pMySqlStmt);

	delete[] pResBind;
}
