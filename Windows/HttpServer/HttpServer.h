#pragma once
#include "iocp.h"

#define HTTP_BAD_REQUEST 400
#define HTTP_OK 200

typedef std::multimap<std::string, std::string> Headers;
typedef std::multimap<std::string, std::string> Params;

class CHttpRequest {
public:
	CHttpRequest() {

	}
	virtual ~CHttpRequest() {

	}

public:
	void setPath(std::string strPath) {
		m_strPath = strPath;
	}
	std::string getPath() {
		return m_strPath;
	}
	void setMethod(std::string strMethod) {
		m_strMethod = strMethod;
	}
	std::string getMethod() {
		return m_strMethod;
	}
	void setHeader(std::string strHeader) {
		
	}
	void setParams(std::string strParams) {

	}

private:
	CHttpRequest(const CHttpRequest&) = delete;
	CHttpRequest& operator=(const CHttpRequest&) = delete;

public:
	std::string m_strMethod;
	std::string m_strPath;
	std::string m_strVersion;
	std::string m_strParams;
	Headers m_mapHeaders;
	Params m_mapParams;
	CSocketContext* m_pContext;
};
class CHttpResponse {
public:
	CHttpResponse()
	{
		m_nStatus = HTTP_BAD_REQUEST;
		m_strVersion = "HTTP/1.1";
		m_strBody = "";
	}
	virtual ~CHttpResponse()
	{

	}

public:
	void setVersion(std::string strVersion) {
		m_strVersion = strVersion;
	}
	std::string getVersion() {
		return m_strVersion;
	}
	void setHeader(std::string strKey, std::string strValue) {

	}

private:
	CHttpResponse(const CHttpResponse&) = delete;
	CHttpResponse& operator=(const CHttpResponse&) = delete;

public:
	int m_nStatus;
	std::string m_strVersion;
	std::string m_strBody;
	Headers m_mapHeaders;
	CSocketContext* m_pContext;
};

typedef std::function<void(CHttpRequest&, CHttpResponse&)> Handler;
typedef std::vector<std::pair<std::regex, Handler>> Handlers;
class CHttpServer :public CIocpTcpServer {
public:
	CHttpServer() : CIocpTcpServer(CPS_FLAG_DEFAULT) {
		registeGetCallback(std::string("echo"), std::bind(&CHttpServer::echoCallback, this, std::placeholders::_1, std::placeholders::_2));
	}
	virtual ~CHttpServer() {

	}

public:
	virtual void OnRequest(void* p1, void* p2)
	{
		NetPacket* pNP = (NetPacket*)p1;
		std::string strSrc((const char*)pNP->GetData());

		int nOffset = 0;
		std::vector<std::string> strStrings;
		for (size_t i = 0; i < strSrc.size(); i++) {
			if ('\r' == strSrc[i]) {
				std::string str = strSrc.substr(nOffset, i - nOffset);
				strStrings.emplace_back(str);
				nOffset = i + 2;
			}
		}

		nOffset = 0;
		std::string strHead = strStrings[0];
		std::vector<std::string> strHeadStrings;
		for (size_t i = 0; i < strHead.size(); i++) {
			if (' ' == strHead[i]) {
				std::string str = strSrc.substr(nOffset, i - nOffset);
				strHeadStrings.emplace_back(str);
				nOffset = i + 2;
			}
		}
		strHeadStrings.emplace_back(strSrc.substr(nOffset - 1, strHead.size() - nOffset + 1));

		CHttpRequest request;
		request.m_pContext = pNP->GetCtx();
		request.setMethod(strHeadStrings[0]);
		request.setPath(strHeadStrings[1]);
		request.m_strVersion = strHeadStrings[2];

		for (auto iter : strStrings)
		{
			auto index = iter.find(':');
			if (iter.npos != index) {
				std::string strHead = iter.substr(0, index);
				std::string strParm = iter.substr(index + 1, iter.size());
				request.m_mapHeaders.insert(std::make_pair(strHead, strParm));
			}
		}

		auto index = strSrc.find("\r\n\r\n");
		if (strSrc.npos != index) {
			request.m_strParams = strSrc.substr(index + 4, (strSrc.size() - index - 3));
		}

		CHttpResponse response;
		response.m_pContext = pNP->GetCtx();
		dispatchRequest(request, response);
	}

private:
	void echoCallback(CHttpRequest& request, CHttpResponse& response) {
		response.m_strBody = "hello world!";
		return httpResponse(response);
	}
	void registeGetCallback(std::string strMethod, Handler handler) {
		m_mapGetIterfaces.insert(std::make_pair(strMethod, handler));
	}
	void dispatchRequest(CHttpRequest& request, CHttpResponse& response) {
		if ("GET" == request.getMethod()) {
			for (auto& iter : m_mapGetIterfaces) {
				if (iter.first == request.getPath()) {
					iter.second(request, response);
				}
			}
		}
		else if ("POST" == request.getMethod()) {

		}
	}

	void httpResponse(CHttpResponse& response)
	{
		char tstr[30];
		time_t now = time(nullptr);
		tm* gmt = gmtime(&now);
		strftime(tstr, sizeof(tstr), "%a, %d %b %Y %H:%M:%S GMT", gmt);
		std::string strTime = tstr;

		std::string strRes = std::string("HTTP/1.1 ");
		strRes += std::to_string(200);
		strRes += " OK\r\n";
		strRes += "Server: CPlusServer/1.0\r\n";
		strRes += "Connection: keep-alive\r\n";
		strRes += strTime;
		strRes += "\r\n\r\n";
		strRes += response.m_strBody;

		SendData(response.m_pContext, strRes.c_str(), strRes.size());
		//CloseClient(response.m_pContext);
	}

private:
	CHttpServer(const CHttpServer&) = delete;
	CHttpServer& operator=(const CHttpServer&) = delete;

private:
	std::multimap<std::string, Handler> m_mapGetIterfaces;
};
