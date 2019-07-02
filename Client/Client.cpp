// Client.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <ctime>
#include <process.h>

#define SAFE_RELEASE_HANDLE(x)               {if(x != NULL && x!=INVALID_HANDLE_VALUE){ CloseHandle(x);x = NULL;}}
#define SAFE_RELEASE(x)                      {if(x != NULL ){delete x;x=NULL;}}

CClient::CClient(char* szServerIP, char* szClientIP, int nPort, int nThreads)/* :
m_szServerIP((char*)DEFAULT_IP),
m_szClientIP((char*)DEFAULT_IP),
m_nPort(DEFAULT_PORT),
m_nThreads(DEFAULT_THREAD),
m_hConnectionThread(NULL),
m_phSendThreads(NULL),
m_pWorkerThreadParam(NULL),
m_hShutdownEvent(NULL)*/
{
	m_szServerIP = szServerIP;
	m_szClientIP = szClientIP;
	m_nPort = nPort;
	m_nThreads = nThreads;
	m_hConnectionThread = NULL;
	m_phSendThreads = NULL;
	m_pWorkerThreadParam = NULL;
	m_hShutdownEvent = NULL;
}

CClient::~CClient()
{
	this->Stop();
}

BOOL CClient::InitialiazeConnection()
{
	unsigned int nSendThreadID;
	unsigned int nRecvThreadID;

	m_phSendThreads = new HANDLE[m_nThreads];
	m_phRecvThreads = new HANDLE[m_nThreads];
	m_pWorkerThreadParam = new WORKER_THREAD_PARAM[m_nThreads];

	//生成指定数量线程连接服务器发送、接收数据
	for (auto i = 0; i < m_nThreads; i++)
	{
		if (WAIT_OBJECT_0 == WaitForSingleObject(m_hShutdownEvent, 0))
		{
			std::cout << "Got Exit Order...Exti...." << std::endl;
			return TRUE;
		}

		//连接服务器
		if (!this->ConnectToServer(&m_pWorkerThreadParam[i].sSocket, m_szServerIP, m_nPort))
		{
			std::cout << "Connect to Server Failed..." << std::endl;
			CleanUp();
			return FALSE;
		}

		m_pWorkerThreadParam[i].nThreadId = i + 1;
		sprintf(m_pWorkerThreadParam[i].szBuffer, "Thread:%d Data:%s", m_pWorkerThreadParam[i].nThreadId, "Hello Server...");
		m_pWorkerThreadParam[i].pClient = this;

		m_phSendThreads[i] = (HANDLE)::_beginthreadex(0, 0, SendThread, (void*)&m_pWorkerThreadParam[i], 0, &nSendThreadID);
		m_phRecvThreads[i] = (HANDLE)::_beginthreadex(0, 0, RecvThread, (void*)&m_pWorkerThreadParam[i], 0, &nRecvThreadID);
	}

	return TRUE;
}

BOOL CClient::ConnectToServer(SOCKET* pSocket, char* pServerIP, int nPort)
{
	LoadSocketLib();

	sockaddr_in siServerAddr;

	*pSocket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == *pSocket)
	{
		std::cout << "Initialize Socket Failed with :" << WSAGetLastError() << std::endl;
		return FALSE;
	}

	ZeroMemory((char*)&siServerAddr, sizeof(siServerAddr));
	siServerAddr.sin_family = AF_INET;
	siServerAddr.sin_addr.S_un.S_addr = inet_addr(pServerIP);
	siServerAddr.sin_port = htons(nPort);

	auto nRet = ::connect(*pSocket, (struct sockaddr*)(&siServerAddr), sizeof(siServerAddr));
	if (SOCKET_ERROR == nRet)
	{
		closesocket(*pSocket);
		std::cout << "Connect to Server Failed..." << errno << std::endl;
		return FALSE;
	}
	std::cout << "Connect to Server Success..." << errno << std::endl;

	return TRUE;
}

BOOL CClient::LoadSocketLib()
{
	WSADATA wsaData;
	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (NO_ERROR != nRet)
	{
		std::cout << "Initialiaze the WinSock2 Failed..." << std::endl;
		return FALSE;
	}

	return TRUE;
}
void CClient::UnloadSocketLib()
{
	WSACleanup();
}


BOOL CClient::Run()
{
	m_hShutdownEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	unsigned int nThreadID = 0;

	WORKER_THREAD_PARAM* pWorkerThreadParams = new WORKER_THREAD_PARAM;
	pWorkerThreadParams->pClient = this;
	m_hConnectionThread = (HANDLE)::_beginthreadex(0, 0, ConnectionThread, (void*)pWorkerThreadParams, 0, &nThreadID);

	return TRUE;
}

void CClient::Stop()
{
	if (NULL == m_hShutdownEvent)
	{
		return;
	}

	SetEvent(m_hShutdownEvent);
	WaitForSingleObject(m_hShutdownEvent, INFINITE);

	// 关闭所有的Socket
	for (auto i = 0; i< m_nThreads; i++)
	{
		closesocket(m_pWorkerThreadParam[i].sSocket);
	}

	//等待所有工作者线程退出
	WaitForMultipleObjects(m_nThreads, m_phSendThreads, TRUE, INFINITE);
	WaitForMultipleObjects(m_nThreads, m_phRecvThreads, TRUE, INFINITE);
	CleanUp();

	std::cout << "Stopped Client..." << std::endl;
}

void CClient::CleanUp()
{
	if (m_hShutdownEvent == NULL) return;

	SAFE_RELEASE(m_phSendThreads);
	SAFE_RELEASE(m_phRecvThreads);

	SAFE_RELEASE_HANDLE(m_hConnectionThread);

	SAFE_RELEASE(m_pWorkerThreadParam);

	SAFE_RELEASE_HANDLE(m_hShutdownEvent);
}

unsigned int __stdcall CClient::ConnectionThread(LPVOID lpParam)
{

	WORKER_THREAD_PARAM* pParam = (WORKER_THREAD_PARAM*)lpParam;
	CClient* pClient = (CClient*)pParam->pClient;
	pClient->InitialiazeConnection();

	SAFE_RELEASE(pParam);
	return 0;
}

unsigned int __stdcall CClient::SendThread(LPVOID lpParam)
{
	WORKER_THREAD_PARAM* pParam = (WORKER_THREAD_PARAM*)lpParam;
	CClient* pClient = (CClient*)pParam->pClient;

	while (TRUE)
	{
		Sleep(2500);
		pClient->SendData(pParam->sSocket, 10001, "hello world!", sizeof("hello world!"));
	}

	return 0;
}

unsigned int __stdcall CClient::RecvThread(LPVOID lpParam)
{
	WORKER_THREAD_PARAM* pParam = (WORKER_THREAD_PARAM*)lpParam;
	CClient* pClient = (CClient*)pParam->pClient;

	while (pClient->m_hShutdownEvent != NULL)
	{
		char recvBuf[8 * MAX_DATA_BUF_SIZE];
		memset(recvBuf, 0, 8 * MAX_DATA_BUF_SIZE);

		auto nRet = ::recv(pParam->sSocket, recvBuf, sizeof(recvBuf), 0);
		if (SOCKET_ERROR == nRet)
		{
			if (WSAEWOULDBLOCK == WSAGetLastError())
			{
				continue;
			}
			std::cout << "Recv Data Failed!" << std::endl;
			return 0;
		}
		else if (0 == nRet)
		{
			return 0;
		}
		std::cout << "Recv Data: [" << recvBuf << "] From " << pClient->m_szServerIP << std::endl;
	}

	return 0;
}

void CClient::SendData(SOCKET socket, const char* pData, int nDataLen)
{
	auto nBytesSend = ::send(socket, pData, nDataLen, 0);
	if (SOCKET_ERROR == nBytesSend)
	{
		std::cout << "Send Message Failed..." << std::endl;
	}
}

void CClient::SendData(SOCKET socket, UINT nRequest, void* pData, int nDataLen)
{
	MESSAGE_HEAD stuMessageHead = { 0 };
	stuMessageHead.hSocket = socket;
	MESSAGE_CONTENT stuMessageContent = { 0 };
	stuMessageContent.nRequest = nRequest;
	stuMessageContent.nDataLen = nDataLen;
	stuMessageContent.pDataPtr = pData;

	CBufferEx myDataPool;
	myDataPool.Write((PBYTE)&stuMessageHead, sizeof(MESSAGE_HEAD));
	myDataPool.Write((PBYTE)&stuMessageContent, sizeof(MESSAGE_CONTENT));
	myDataPool.Write((PBYTE)pData, nDataLen);

	SendData(socket, (char*)myDataPool.c_Bytes(), myDataPool.GetLength());
}

int _tmain(int argc, _TCHAR* argv[])
{
	for (auto i = 0; i < 1; i++)
	{
		CClient* pClient = new CClient("127.0.0.1", "127.0.0.1", 8888, 1);
		pClient->Run();
	}

	getchar();
	return 0;
}