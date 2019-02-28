#include "stdafx.h"

CHttpRequest::CHttpRequest(const char* szHostName, unsigned short usPort)
{
	m_szHostName = szHostName;
	m_usPort = usPort;
	m_nErrorCode = 0;
}
CHttpRequest::~CHttpRequest()
{

}

void CHttpRequest::PostHttpRequest(enHTTP_OPERATION enHttpOperation)
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	struct hostent* host_addr = gethostbyname(m_szHostName);
	if (NULL == host_addr)
	{
		m_nErrorCode = enNullHost;
		return;
	}

	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons((unsigned short)m_usPort);
	sin.sin_addr.s_addr = *((int*)*host_addr->h_addr_list);

	SOCKET socket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (SOCKET_ERROR == socket)
	{
		m_nErrorCode = enSocketError;
		return;
	}

	int nRet = ::connect(socket, (const struct sockaddr *)&sin, sizeof(sockaddr_in));
	if (nRet < 0)
	{
		m_nErrorCode = enConnetError;
		return;
	}

	nRet = ::send(socket, m_strSendStr.c_str(), m_strSendStr.length(), 0);
	if (nRet < 0)
	{
		m_nErrorCode = enSendFailed;
		return;
	}

	char szRecv[4096] = { 0 };
	nRet = ::recv(socket, szRecv, sizeof(szRecv), 0);
	if (nRet < 0)
	{
		m_nErrorCode = enRecvFailed;
		return;
	}

	m_strRecvStr = szRecv;
	printf("%s\n", m_strRecvStr.c_str());
	
	WSACleanup();
	closesocket(socket);
}

void CHttpRequest::HttpGet(std::string api, int nParamDataLen)
{
	char szSend[4096] = { 0 };
	sprintf_s(szSend, "GET %s HTTP/1.1\r\n\
					Host: %s\r\n\
					Connection: keep-alive\r\n\
					Content-Length: %d\r\n\
					Cache-Control: max-age=0\r\n\
					User-Agent: Mozilla/5.0 (Windows NT 6.1) AppleWebKit/535.1 (KHTML, like Gecko) Chrome/15.0.849.0 Safari/535.1\r\n\
					Content-Type: application/x-www-form-urlencoded\r\n\
					Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n\
					Accept-Encoding: gzip,deflate,sdch\r\n\
					Accept-Language: zh-CN,zh;q=0.8\r\n", api, m_szHostName, nParamDataLen);
	m_strSendStr = szSend;
	PostHttpRequest(enHTTP_GET);
}

void CHttpRequest::HttpPost(std::string api, std::string param)
{
	char szSend[4096] = { 0 };
	sprintf_s(szSend, "POST %s HTTP/1.1\r\n\
					Host: %s\r\n\
					Connection: keep-alive\r\n\
					Content-Length: %d\r\n\
					Cache-Control: max-age=0\r\n\
					User-Agent: Mozilla/5.0 (Windows NT 6.1) AppleWebKit/535.1 (KHTML, like Gecko) Chrome/15.0.849.0 Safari/535.1\r\n\
					Content-Type: application/x-www-form-urlencoded\r\n\
					Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n\
					Accept-Encoding: gzip,deflate,sdch\r\n\
					Accept-Language: zh-CN,zh;q=0.8\r\n", api, m_szHostName, param.length());
	m_strSendStr = szSend;
	PostHttpRequest(enHTTP_POST);
}