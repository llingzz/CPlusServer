#include "stdafx.h"

CMysqlRecordSet::CMysqlRecordSet()
{
	m_pBinds = nullptr;
	m_nRowCount = 0;
	m_nFieldNum = 0;
}

CMysqlRecordSet::~CMysqlRecordSet()
{
	m_pBinds = nullptr;
	m_nRowCount = 0;
	m_nFieldNum = 0;
}

void CMysqlRecordSet::SetFieldType(int nIndex, enum_field_types enType)
{
	MYSQL_BIND* pTemp = &m_pBinds[nIndex];

	unsigned long lBufferSize = 0;
	switch (enType)
	{
	case MYSQL_TYPE_DECIMAL:
		break;
	case MYSQL_TYPE_TINY:
		lBufferSize = 1;
		break;
	case MYSQL_TYPE_SHORT:
		lBufferSize = 2;
		break;
	case MYSQL_TYPE_LONG:
		lBufferSize = 4;
		break;
	case MYSQL_TYPE_FLOAT:
		lBufferSize = 4;
		break;
	case MYSQL_TYPE_DOUBLE:
		lBufferSize = 8;
		break;
	case MYSQL_TYPE_NULL:
		break;
	case MYSQL_TYPE_TIMESTAMP:
		lBufferSize = 8;
		break;
	case MYSQL_TYPE_LONGLONG:
		lBufferSize = 8;
		break;
	case MYSQL_TYPE_INT24:
		lBufferSize = 4;
		break;
	case MYSQL_TYPE_DATE:
		lBufferSize = 3;
		break;
	case MYSQL_TYPE_TIME:
		lBufferSize = 3;
		break;
	case MYSQL_TYPE_DATETIME:
		lBufferSize = 8;
		break;
	case MYSQL_TYPE_YEAR:
		lBufferSize = 1;
		break;
	case MYSQL_TYPE_NEWDATE:
		lBufferSize = 4;
		break;
	case MYSQL_TYPE_VARCHAR:
		lBufferSize = 255;
		break;
	case MYSQL_TYPE_BIT:
		lBufferSize = 1;
		break;
	case MYSQL_TYPE_TIMESTAMP2:
		break;
	case MYSQL_TYPE_DATETIME2:
		break;
	case MYSQL_TYPE_TIME2:
		break;
	case MYSQL_TYPE_JSON:
		break;
	case MYSQL_TYPE_NEWDECIMAL:
		break;
	case MYSQL_TYPE_ENUM:
		lBufferSize = 65535;
		break;
	case MYSQL_TYPE_SET:
		lBufferSize = 64;
		break;
	case MYSQL_TYPE_TINY_BLOB:
		lBufferSize = 256;
		break;
	case MYSQL_TYPE_MEDIUM_BLOB:
		lBufferSize = 16777215;
		break;
	case MYSQL_TYPE_LONG_BLOB:
		lBufferSize = 4294967295;
		break;
	case MYSQL_TYPE_BLOB:
		lBufferSize = 65535;
		break;
	case MYSQL_TYPE_VAR_STRING:
		lBufferSize = 65535;
		break;
	case MYSQL_TYPE_STRING:
		lBufferSize = 255;
		break;
	case MYSQL_TYPE_GEOMETRY:
		break;
	default:
		break;
	}

	unsigned long lLen = 0;
	pTemp->buffer = malloc(lBufferSize);
	pTemp->buffer_length = lBufferSize;
	pTemp->buffer_type = enType;
	pTemp->length = &lLen;
}

bool CMysqlRecordSet::InitRecordSet(MYSQL_STMT* pMySqlStmt, MYSQL_RES* pResult)
{
	if (pResult)
	{
		int nIndex = 0;
		MYSQL_FIELD* pField = mysql_fetch_field(pResult);
		while (pField)
		{
			m_FieldIndex[pField->name] = nIndex;
			SetFieldType(nIndex, pField->type);
			pField = mysql_fetch_field(pResult);
			nIndex++;
		}
	}

	if (pMySqlStmt)
	{
		if (m_pBinds)
		{
			SAFE_DELETE_ARRAY(m_pBinds);
		}

		m_nRowCount = mysql_stmt_num_rows(pMySqlStmt);
		m_nFieldNum = mysql_stmt_field_count(pMySqlStmt);
		if (m_nFieldNum > 0)
		{
			m_pBinds = new MYSQL_BIND[m_nFieldNum];
			memset(m_pBinds, 0, sizeof(MYSQL_BIND)* m_nFieldNum);
		}

		if (0 != mysql_stmt_bind_result(pMySqlStmt, m_pBinds))
		{
			mysql_stmt_close(pMySqlStmt);
		}
	}
}

bool CMysqlRecordSet::ClearRecordSet()
{
	for (int i = 0; i < m_nFieldNum; i++)
	{
		MYSQL_BIND* pTemp = &m_pBinds[i];
		if (pTemp)
		{
			free(pTemp->buffer);
			pTemp->buffer = nullptr;
		}
	}
	SAFE_DELETE_ARRAY(m_pBinds);

	m_nRowCount = 0;
	m_nFieldNum = 0;

	return true;
}

bool CMysqlRecordSet::GetBool(int nIndex)
{
	MYSQL_BIND* pTemp = &m_pBinds[nIndex];
	if (pTemp->is_null_value)
	{
		return false;
	}

	bool bRet = false;
	switch (pTemp->buffer_type)
	{
	case MYSQL_TYPE_TINY:
		bRet = (0 != (*(unsigned char*)pTemp->buffer));
		break;
	case MYSQL_TYPE_SHORT:
		bRet = (0 != (*(unsigned short*)pTemp->buffer));
		break;
	case MYSQL_TYPE_INT24:
		bRet = (0 != (*(unsigned int*)pTemp->buffer));
		break;
	case MYSQL_TYPE_LONG:
		bRet = (0 != (*(unsigned int*)pTemp->buffer));
		break;
	case MYSQL_TYPE_LONGLONG:
		bRet = (0 != (*(unsigned long long*)pTemp->buffer));
		break;
	case MYSQL_TYPE_FLOAT:
		bRet = (0 != (*(unsigned int*)pTemp->buffer));
		break;
	case MYSQL_TYPE_DOUBLE:
		bRet = (0 != (*(unsigned long long*)pTemp->buffer));
		break;
	default:
		break;
	}
	return bRet;
}

bool CMysqlRecordSet::GetBool(std::string strField)
{
	int nIndex = m_FieldIndex[strField];
	return GetBool(nIndex);
}

int CMysqlRecordSet::GetInt(int nIndex)
{
	MYSQL_BIND* pTemp = &m_pBinds[nIndex];
	if (pTemp->is_null_value)
	{
		return 0;
	}

	int nRet = 0;
	switch (pTemp->buffer_type)
	{
	case MYSQL_TYPE_TINY:
		nRet = (int)((*(unsigned char*)pTemp->buffer));
		break;
	case MYSQL_TYPE_SHORT:
		nRet = (int)((*(unsigned short*)pTemp->buffer));
		break;
	case MYSQL_TYPE_INT24:
		nRet = (int)((*(unsigned int*)pTemp->buffer));
		break;
	case MYSQL_TYPE_LONG:
		nRet = (int)((*(unsigned int*)pTemp->buffer));
		break;
	case MYSQL_TYPE_LONGLONG:
		nRet = (int)((*(unsigned long long*)pTemp->buffer));
		break;
	case MYSQL_TYPE_FLOAT:
		nRet = (int)((*(unsigned int*)pTemp->buffer));
		break;
	case MYSQL_TYPE_DOUBLE:
		nRet = (int)((*(unsigned long long*)pTemp->buffer));
		break;
	default:
		break;
	}
	return nRet;
}

int CMysqlRecordSet::GetInt(std::string strField)
{
	int nIndex = m_FieldIndex[strField];
	return GetInt(nIndex);
}

float CMysqlRecordSet::GetFloat(int nIndex)
{
	MYSQL_BIND* pTemp = &m_pBinds[nIndex];
	if (pTemp->is_null_value)
	{
		return .0f;
	}

	float fRet = .0f;
	switch (pTemp->buffer_type)
	{
	case MYSQL_TYPE_TINY:
		fRet = (float)((*(unsigned char*)pTemp->buffer));
		break;
	case MYSQL_TYPE_SHORT:
		fRet = (float)((*(unsigned short*)pTemp->buffer));
		break;
	case MYSQL_TYPE_INT24:
		fRet = (float)((*(unsigned int*)pTemp->buffer));
		break;
	case MYSQL_TYPE_LONG:
		fRet = (float)((*(unsigned int*)pTemp->buffer));
		break;
	case MYSQL_TYPE_LONGLONG:
		fRet = (float)((*(unsigned long long*)pTemp->buffer));
		break;
	case MYSQL_TYPE_FLOAT:
		fRet = (float)((*(unsigned int*)pTemp->buffer));
		break;
	case MYSQL_TYPE_DOUBLE:
		fRet = (float)((*(unsigned long long*)pTemp->buffer));
		break;
	default:
		break;
	}
	return fRet;
}

float CMysqlRecordSet::GetFloat(std::string strField)
{
	int nIndex = m_FieldIndex[strField];
	return GetFloat(nIndex);
}

double CMysqlRecordSet::GetDouble(int nIndex)
{
	MYSQL_BIND* pTemp = &m_pBinds[nIndex];
	if (pTemp->is_null_value)
	{
		return 0;
	}

	double dRet = 0;
	switch (pTemp->buffer_type)
	{
	case MYSQL_TYPE_TINY:
		dRet = (double)((*(unsigned char*)pTemp->buffer));
		break;
	case MYSQL_TYPE_SHORT:
		dRet = (double)((*(unsigned short*)pTemp->buffer));
		break;
	case MYSQL_TYPE_INT24:
		dRet = (double)((*(unsigned int*)pTemp->buffer));
		break;
	case MYSQL_TYPE_LONG:
		dRet = (double)((*(unsigned int*)pTemp->buffer));
		break;
	case MYSQL_TYPE_LONGLONG:
		dRet = (double)((*(unsigned long long*)pTemp->buffer));
		break;
	case MYSQL_TYPE_FLOAT:
		dRet = (double)((*(unsigned int*)pTemp->buffer));
		break;
	case MYSQL_TYPE_DOUBLE:
		dRet = (double)((*(unsigned long long*)pTemp->buffer));
		break;
	default:
		break;
	}
	return dRet;
}

double CMysqlRecordSet::GetDouble(std::string strField)
{
	int nIndex = m_FieldIndex[strField];
	return GetDouble(nIndex);
}

std::string CMysqlRecordSet::GetString(int nIndex)
{
	std::string strRet = "";
	MYSQL_BIND* pTemp = &m_pBinds[nIndex];
	if (pTemp->is_null_value)
	{
		return strRet;
	}

	strRet = (char*)pTemp->buffer;
	return strRet;
}

std::string CMysqlRecordSet::GetString(std::string strField)
{
	int nIndex = m_FieldIndex[strField];
	return GetString(nIndex);
}

CMysqlConnection::CMysqlConnection()
{

}

CMysqlConnection::~CMysqlConnection()
{

}

bool CMysqlConnection::Init()
{
	if (0 != mysql_library_init(0, nullptr, nullptr))
	{
		return false;
	}
	if (0 != mysql_thread_init())
	{
		return false;
	}
	return true;
}

void CMysqlConnection::UnInit()
{
	mysql_thread_end();
	mysql_library_end();
}

bool CMysqlConnection::Connect(std::string strHost, std::string strUser, std::string strPwd, std::string strDB, int nPort, std::string strCharset /* = "utf8"*/)
{
	if (m_pMySql)
	{
		mysql_close(m_pMySql);
		SAFE_DELETE(m_pMySql);
	}

	m_pMySql = mysql_init(nullptr);
	if (!m_pMySql)
	{
		return false;
	}

	if (0 != mysql_options(m_pMySql, MYSQL_SET_CHARSET_NAME, strCharset.c_str()))
	{
		unsigned int nErrno = mysql_errno(m_pMySql);
		std::string strError = mysql_error(m_pMySql);
		myLogConsoleE("%s nErrno %d strError %s", __FUNCTION__, nErrno, strError.c_str());

		mysql_close(m_pMySql);
		SAFE_DELETE(m_pMySql);
		return false;
	}

	if (!mysql_real_connect(m_pMySql, strHost.c_str(), strUser.c_str(), strPwd.c_str(), strDB.c_str(), nPort, nullptr, 0))
	{
		unsigned int nErrno = mysql_errno(m_pMySql);
		std::string strError = mysql_error(m_pMySql);
		myLogConsoleE("%s nErrno %d strError %s", __FUNCTION__, nErrno, strError.c_str());

		mysql_close(m_pMySql);
		SAFE_DELETE(m_pMySql);
		return false;
	}

	m_strHost.assign(strHost);
	m_strUser.assign(strUser);
	m_strPwd.assign(strPwd);
	m_strDB.assign(strDB);
	m_strCharSet.assign(strCharset);

	m_nPort = nPort;

	return true;
}

bool CMysqlConnection::Reconnect()
{
	if (m_pMySql && 0 == mysql_ping(m_pMySql))
	{
		return true;
	}

	return Connect(m_strHost, m_strUser, m_strPwd, m_strDB, m_nPort, m_strCharSet);
}

int CMysqlConnection::Excute(std::string strSql, CMysqlRecordSet& recordSet)
{
	int nRet = mysql_real_query(m_pMySql, strSql.c_str(), strSql.size());
	if (0 != nRet)
	{
		int nError = mysql_errno(m_pMySql);
		if (nError == CR_SERVER_GONE_ERROR || nError == CR_SERVER_LOST)
		{
			if (Reconnect())
			{
				nRet = mysql_real_query(m_pMySql, strSql.c_str(), strSql.size());
				if (0 != nRet)
				{
					return 0;
				}
				return (int)mysql_affected_rows(m_pMySql);
			}
			else
			{
				return 0;
			}
		}
	}

	return (int)mysql_affected_rows(m_pMySql);
}

void CMysqlConnection::Query(std::string strSql, CMysqlRecordSet& recordSet)
{
	int nRet = mysql_real_query(m_pMySql, strSql.c_str(), strSql.size());
	if (0 != nRet)
	{
		int nError = mysql_errno(m_pMySql);
		if (nError == CR_SERVER_GONE_ERROR || nError == CR_SERVER_LOST)
		{
			if (Reconnect())
			{
				nRet = mysql_real_query(m_pMySql, strSql.c_str(), strSql.size());
			}
			else
			{
				return;
			}
		}
		return;
	}
	if (0 != nRet)
	{
		return;
	}
	recordSet.InitRecordSet = mysql_store_result(m_pMySql);
}

void CMysqlConnection::Close()
{
	if (m_pMySql)
	{
		mysql_close(m_pMySql);
		SAFE_DELETE(m_pMySql);
	}
}