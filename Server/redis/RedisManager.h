#pragma once

#include "../redis/redis/hiredis.h"

struct redisContext;
struct redisReply;

class CAutoReleaseRedisReply{
public:
	CAutoReleaseRedisReply(redisReply** pRedisReply)
		:m_pRedisReply(pRedisReply)
	{

	}
	~CAutoReleaseRedisReply()
	{
		ReleaseRedisReply();
	}

	void SetRedisReplay(redisReply **pRedisReply)
	{
		m_pRedisReply = pRedisReply;
	}

	void ReleaseRedisReply()
	{
		if (*m_pRedisReply)
		{
			freeReplyObject(*m_pRedisReply);
			*m_pRedisReply = NULL;
		}
	}

private:
	redisReply**	m_pRedisReply;
};

typedef struct _tagRedisData
{
	int		nType;
	__int64 n64Data;
	char strData[1024];
}REDIS_DATA, LPREDIS_DATA;

typedef std::list<REDIS_DATA> CRedisDataList;

class CRedisManager{
public:
	CRedisManager();
	CRedisManager(char* szIP, int nPort, int nDBIndex, int nTimeOutSec = 1);
	virtual ~CRedisManager();

public:
	BOOL InitServer(char* szIP, int nPort, char* szPW, int nDBIndex, int nTimeOutSec = 1);

	BOOL ConnectServer();
	BOOL ConnectServer(char* szIP, int nPort, char* szPW, int nDBIndex, int nTimeOutSec = 1);
	void DisConnect();

	BOOL Authenticate();

	BOOL SelectDB(int nIndex);
	BOOL SelectDB();
	BOOL __SelectDB(int nIndex);

	BOOL RedisCommand(REDIS_DATA& stuResult, const char* format, ...);
	BOOL RedisCommand(CRedisDataList& lstResult, const char* format, ...);

	BOOL GetResultFromReply(const redisReply* reply, REDIS_DATA& stuRedisData);

	BOOL PingServer();
	void Test();

	BOOL IsRedisDBIndexRight(CRedisManager* pRedisManager);
	BOOL IsDBIndexRight();

	BOOL _RD_HGET(const char szKey[], const char szFiled[], REDIS_DATA& stuRedisData);

protected:
	char*			m_szIP;
	char*			m_szPW;
	int				m_nPort;
	int             m_nTimeOutSec;

	int				m_nDBIndexTo;
	int				m_nCurrentDBIndex;

	CritSec			m_csRedisCmd;
	redisContext*	m_pRedisContext;

	CritSec			m_csRedisReply;
	redisReply*		m_pRedisReply;
};

static BOOL RD_HGET(CRedisManager* pRedisManager, const char szKey[], const char szFiled[], REDIS_DATA& stuRedisData);