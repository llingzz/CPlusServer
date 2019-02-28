#pragma once

typedef enum enHTTP_CODE{
	enNullHost = -100,
	enSocketError = -101,
	enConnetError = -102,
	enSendFailed = -103,
	enRecvFailed = -104,
	enDefault = 0,
};

typedef enum enHTTP_OPERATION{
	enHTTP_DEFAULT = 0,
	enHTTP_GET,
	enHTTP_POST,
};

class CHttpRequest{
public:
	CHttpRequest(const char* szHostName, unsigned short usPort);
	~CHttpRequest();

public:
	void PostHttpRequest(enHTTP_OPERATION enHttpOperation);

	void HttpGet(std::string api, int nParamDataLen);
	void HttpPost(std::string api, std::string param);

public:
	const char* m_szHostName;
	unsigned short m_usPort;
	std::string m_strSendStr;
	std::string m_strRecvStr;
	int m_nErrorCode;
};