#include "stdafx.h"

CRedisManager::CRedisManager()
{
	m_nPort = 0;
	m_nTimeOutSec = 0;
	m_pRedisContext = NULL;
	m_nDBIndexTo = 0;
	m_nCurrentDBIndex = 0;
	m_pRedisReply = NULL;
}
CRedisManager::CRedisManager(char* szIP, int nPort, int nDBIndex, int nTimeOutSec /* = 1 */)
{
	m_szIP = szIP;
	m_nPort = nPort;
	m_nTimeOutSec = nTimeOutSec;
	m_pRedisContext = NULL;
	m_nDBIndexTo = nDBIndex;
	m_nCurrentDBIndex = 0;
	m_pRedisReply = NULL;
}
CRedisManager::~CRedisManager()
{
	if (m_pRedisContext)
	{
		DisConnect();
	}
}

BOOL CRedisManager::InitServer(char* szIP, int nPort, char* szPW, int nDBIndex, int nTimeOutSec /* = 1 */)
{
	m_szIP = szIP;
	m_nPort = nPort;
	m_nTimeOutSec = nTimeOutSec;
	m_pRedisContext = NULL;
	m_nDBIndexTo = nDBIndex;
	m_nCurrentDBIndex = 0;
	m_szPW = szPW;

	return TRUE;
}

BOOL CRedisManager::ConnectServer()
{
	if (!m_szIP)
	{
		return FALSE;
	}

	struct timeval t = { m_nTimeOutSec / 1000, m_nTimeOutSec * 1000 }; //1.0 Ãë
	m_pRedisContext = redisConnectWithTimeout((LPSTR)(LPCTSTR)m_szIP, m_nPort, t);
	//m_pRedisContext = redisConnect((LPSTR)(LPCTSTR)m_szIP, m_nPort);
	if (!m_pRedisContext)
	{
		return FALSE;
	}

	if (m_pRedisContext && m_pRedisContext->err)
	{
		DisConnect();
		return FALSE;
	}

	if (Authenticate() && __SelectDB(m_nDBIndexTo))
	{
		return TRUE;
	}

	DisConnect();
	return FALSE;
}

BOOL CRedisManager::ConnectServer(char* szIP, int nPort, char* szPW, int nDBIndex, int nTimeOutSec /* = 1 */)
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (InitServer(szIP, nPort, szPW, nDBIndex, nTimeOutSec))
	{
		return ConnectServer();
	}

	return FALSE;
}

BOOL CRedisManager::Authenticate()
{
	if (m_szIP)
	{
		m_csRedisCmd.Lock();

		if (NULL == m_pRedisContext)
		{
			return FALSE;
		}

		auto reply = (redisReply*)redisCommand(m_pRedisContext, "AUTH %s", m_szPW);
		CAutoReleaseRedisReply autoReleaseReply(&reply);

		if (m_pRedisContext->err != REDIS_OK)
		{
			DisConnect();
			return FALSE;
		}

		if (reply)
		{
			if (REDIS_REPLY_STATUS == reply->type)
			{
				string strRet = reply->str;
				if (strRet.compare("OK") == 0)
				{
					return TRUE;
				}
			}
			else
			{
				//TODO:
			}
		}
		else
		{
			//TODO:
		}

		m_csRedisCmd.Unlock();
	}
	else
	{
		//TODO:
	}

	return FALSE;
}

void CRedisManager::DisConnect()
{
	//CAutoLock lock(&m_csRedisCmd);
	if (m_pRedisContext)
	{
		redisFree(m_pRedisContext);
		m_pRedisContext = NULL;
	}

	m_nCurrentDBIndex = 0;
}

BOOL CRedisManager::SelectDB(int nIndex)
{
	//CAutoLock lock(&m_csRedisCmd);
	if (NULL == m_pRedisContext)
	{
		if (!ConnectServer())
		{
			return FALSE;
		}
	}

	return __SelectDB(nIndex);
}

BOOL CRedisManager::SelectDB()
{
	//CAutoLock lock(&m_csRedisCmd);
	if (NULL == m_pRedisContext)
	{
		if (!ConnectServer())
		{
			return FALSE;
		}
	}

	return __SelectDB(m_nDBIndexTo);
}

BOOL CRedisManager::__SelectDB(int nIndex)
{
	//CAutoLock lock(&m_csRedisCmd);
	/*CString test = "";
	test.Format(_T("select %u"), nIndex);

	char* testChar = new char;
	int size = WideCharToMultiByte(CP_ACP, 0, test, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, test, -1, testChar, size, 0, 0);*/

	m_csRedisCmd.Lock();
	if (NULL == m_pRedisContext)
	{
		return FALSE;
	}

	//auto reply = (redisReply*)redisCommand(m_pRedisContext, testChar);
	auto reply = (redisReply*)redisCommand(m_pRedisContext, "select %s", "0");
	/*delete testChar;
	testChar = NULL;*/

	CAutoReleaseRedisReply stuAutoReleaseObj(&reply);

	if (m_pRedisContext->err != REDIS_OK)
	{
		return FALSE;
	}

	if (REDIS_REPLY_STATUS == reply->type)
	{
		string strRet = reply->str;
		if (strRet.compare("OK") == 0)
		{
			m_nDBIndexTo = nIndex;
			m_nCurrentDBIndex = nIndex;
			return TRUE;
		}
		else
		{

		}
	}
	else
	{

	}
	m_csRedisCmd.Unlock();
	return FALSE;
}

BOOL CRedisManager::RedisCommand(REDIS_DATA& stuResult, const char* format, ...)
{
	if (FALSE == IsRedisDBIndexRight(this))
	{
		return FALSE;
	}

	//CAutoLock lock(&m_csRedisCmd);
	if (NULL == m_pRedisContext)
	{
		if (FALSE == ConnectServer())
		{
			return FALSE;
		}
	}

	redisReply* reply = (redisReply*)redisCommand(m_pRedisContext, format);
	CAutoReleaseRedisReply stuAutoReleaseObj(&reply);

	if (m_pRedisContext->err != REDIS_OK)
	{
		stuAutoReleaseObj.ReleaseRedisReply();

		DisConnect();

		if (FALSE == ConnectServer())
		{
			return FALSE;
		}

		reply = (redisReply*)redisCommand(m_pRedisContext, format);
		stuAutoReleaseObj.SetRedisReplay(&reply);
	}

	if (m_pRedisContext->err != REDIS_OK)
	{
		return FALSE;
	}

	if (!GetResultFromReply(reply, stuResult))
	{
		return FALSE;
	}
	stuResult;
	return TRUE;
}

BOOL CRedisManager::RedisCommand(CRedisDataList& lstResult, const char* format, ...)
{
	if (FALSE == IsRedisDBIndexRight(this))
	{
		return FALSE;
	}

	//CAutoLock lock(&m_csRedisCmd);
	if (NULL == m_pRedisContext)
	{
		if (FALSE == ConnectServer())
		{
			return FALSE;
		}
	}

	redisReply* reply = (redisReply*)redisCommand(m_pRedisContext, format);
	CAutoReleaseRedisReply stuAutoReleaseObj(&reply);

	if (m_pRedisContext->err != REDIS_OK)
	{
		stuAutoReleaseObj.ReleaseRedisReply();

		DisConnect();

		if (FALSE == ConnectServer())
		{
			return FALSE;
		}

		reply = (redisReply*)redisCommand(m_pRedisContext, format);
		stuAutoReleaseObj.SetRedisReplay(&reply);
	}

	if (m_pRedisContext->err != REDIS_OK)
	{
		return FALSE;
	}

	if (!reply || REDIS_REPLY_ARRAY != reply->type)
	{
		return FALSE;
	}

	for (size_t i = 0; i < reply->elements; i++)
	{
		REDIS_DATA stuRedisData;
		auto bRet = GetResultFromReply(reply->element[i], stuRedisData);
		if (bRet)
		{
			lstResult.push_back(stuRedisData);
		}
	}

	return TRUE;
}

BOOL CRedisManager::GetResultFromReply(const redisReply* reply, REDIS_DATA& stuRedisData)
{
	stuRedisData.nType = 0;
	stuRedisData.n64Data = 0;
	//stuRedisData.strData = "";
	strcpy(stuRedisData.strData, "");

	if (NULL == reply)
	{
		return FALSE;
	}

	stuRedisData.nType = reply->type;

	if (REDIS_REPLY_STRING == reply->type || REDIS_REPLY_STATUS == reply->type || REDIS_REPLY_ERROR == reply->type)
	{
		//stuRedisData.strData = reply->str;
		strcpy(stuRedisData.strData, reply->str);
	}
	else if (REDIS_REPLY_NIL == reply->type)
	{
		//stuRedisData.strData = "";
		strcpy(stuRedisData.strData, "");
	}
	else if (REDIS_REPLY_INTEGER == reply->type)
	{
		stuRedisData.n64Data = reply->integer;
	}
	else
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CRedisManager::IsRedisDBIndexRight(CRedisManager* pRedisManager)
{
	if (!pRedisManager)
	{
		return FALSE;
	}

	if (FALSE == pRedisManager->IsDBIndexRight())
	{
		if (FALSE == pRedisManager->SelectDB())
		{
			pRedisManager->DisConnect();
			return pRedisManager->ConnectServer();
		}
	}

	return TRUE;
}

BOOL CRedisManager::IsDBIndexRight()
{
	return m_nCurrentDBIndex == m_nDBIndexTo ? TRUE : FALSE;
}

BOOL CRedisManager::PingServer()
{
	//CAutoLock lock(&m_csRedisCmd);

	if (NULL == m_pRedisContext)
	{
		if (FALSE == ConnectServer())
		{
			return FALSE;
		}
	}

	auto reply = (redisReply *)redisCommand(m_pRedisContext, "PING");

	CAutoReleaseRedisReply stuAutoReleaseObj(&reply);

	if (m_pRedisContext->err != REDIS_OK)
	{
		return FALSE;
	}

	if (reply)
	{
		if (REDIS_REPLY_STATUS == reply->type)
		{
			string strRet = reply->str;
			if (strRet.compare("PONG") == 0)
			{
				std::cout << "PING PONG!" << std::endl;
				return TRUE;
			}
			else
			{
			}
		}
		else
		{
		}
	}
	else
	{
	}

	return FALSE;
}

void CRedisManager::Test()
{
	REDIS_DATA stuRedisData = { 0 };
	RD_HGET(this, "x", "x", stuRedisData);
	//LOG_INFO(stuRedisData.strData);
	cout << stuRedisData.strData << endl;
}

BOOL CRedisManager::_RD_HGET(const char szKey[], const char szFiled[], REDIS_DATA& stuRedisData)
{
	char* redisCmd = new char[1024];
	sprintf(redisCmd, "HGET %s %s", szKey, szFiled);

	if (FALSE == this->RedisCommand(stuRedisData, redisCmd))
	{
		//LOG_ERROR("Redis Excute HGET redisCommand Failed with redisCmd = " << redisCmd);
		//SAFE_DELETE_ARRAY(redisCmd);
		return FALSE;
	}
	stuRedisData;
	if (REDIS_REPLY_STRING == stuRedisData.nType)
	{
		//SAFE_DELETE_ARRAY(redisCmd);
		return TRUE;
	}
	else
	{
		//LOG_ERROR("Redis HGET Error with redisCmd = " << redisCmd << " stuRedisData.nType = " << stuRedisData.nType << " stuRedisData.strData = " << stuRedisData.strData);
		//SAFE_DELETE_ARRAY(redisCmd);
		return FALSE;
	}

	//SAFE_DELETE_ARRAY(redisCmd);
	return FALSE;
}

BOOL RD_HGET(CRedisManager* pRedisManager, const char szKey[], const char szFiled[], REDIS_DATA& stuRedisData)
{
	char* redisCmd = new char[1024];
	sprintf(redisCmd, "HGET %s %s", szKey, szFiled);

	if (FALSE == pRedisManager->RedisCommand(stuRedisData, redisCmd))
	{
		//LOG_ERROR("Redis Excute HGET redisCommand Failed with redisCmd = " << redisCmd);
		//SAFE_DELETE_ARRAY(redisCmd);
		return FALSE;
	}
	stuRedisData;
	if (REDIS_REPLY_STRING == stuRedisData.nType)
	{
		//SAFE_DELETE_ARRAY(redisCmd);
		return TRUE;
	}
	else
	{
		//LOG_ERROR("Redis HGET Error with redisCmd = " << redisCmd << " stuRedisData.nType = " << stuRedisData.nType << " stuRedisData.strData = " << stuRedisData.strData);
		//SAFE_DELETE_ARRAY(redisCmd);
		return FALSE;
	}

	//SAFE_DELETE_ARRAY(redisCmd);
	return FALSE;
}