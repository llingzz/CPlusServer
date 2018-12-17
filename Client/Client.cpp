// Client.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <process.h>

#define SAFE_RELEASE_HANDLE(x)               {if(x != NULL && x!=INVALID_HANDLE_VALUE){ CloseHandle(x);x = NULL;}}
#define SAFE_RELEASE(x)                      {if(x != NULL ){delete x;x=NULL;}}

CClient::CClient() :
m_szServerIP((char*)DEFAULT_IP),
m_szClientIP((char*)DEFAULT_IP),
m_nPort(DEFAULT_PORT),
m_nThreads(DEFAULT_THREAD),
m_hConnectionThread(NULL),
m_phSendThreads(NULL),
m_pWorkerThreadParam(NULL),
m_hShutdownEvent(NULL)
{
}

CClient::~CClient()
{
	this->Stop();
}

BOOL CClient::InitialiazeConnection()
{
	unsigned int nThreadID;

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

		m_phRecvThreads[i] = (HANDLE)::_beginthreadex(0, 0, RecvThread, (void*)&m_pWorkerThreadParam[i], 0, &nThreadID);
#if 1
		//m_phSendThreads[i] = (HANDLE)::_beginthreadex(0, 0, SendThread, (void*)&m_pWorkerThreadParam[i], 0, &nThreadID);
		char test[] = "test";
		while (1)
		{
			SendData(m_pWorkerThreadParam[i].sSocket, 10000, test, sizeof(test));
			Sleep(5000);
		}
#else
		auto nBytesSend = 0;
		char szSend[MAX_DATA_BUF_SIZE];
		memset(szSend, 0, sizeof(szSend));

		sprintf(szSend, "%s", /*pParam->szBuffer*/"Hello Server!");
		printf("%s\n", szSend);
		nBytesSend = ::send(m_pWorkerThreadParam[i].sSocket, szSend, strlen(szSend), 0);
		if (SOCKET_ERROR == nBytesSend)
		{
			std::cout << "Send Message Failed..." << std::endl;
			return 1;
		}
		//std::cout << "Send First Data to Server Successful..." << std::endl;

		Sleep(100);

		memset(szSend, 0, sizeof(szSend));
		sprintf(szSend, "%s", "Hello Server! See you Again!");
		printf("%s\n", szSend);
		nBytesSend = ::send(m_pWorkerThreadParam[i].sSocket, szSend, strlen(szSend), 0);
		if (SOCKET_ERROR == nBytesSend)
		{
			std::cout << "Send Message Failed..." << std::endl;
			return 1;
		}
#endif
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
	siServerAddr.sin_addr.S_un.S_addr = inet_addr(DEFAULT_IP);
	siServerAddr.sin_port = htons(nPort);

	auto nRet = ::connect(*pSocket, (struct sockaddr*)(&siServerAddr), sizeof(siServerAddr));
	if (SOCKET_ERROR == nRet)
	{
		closesocket(*pSocket);
		std::cout << "Connect to Server Failed..." << errno << std::endl;
		return FALSE;
	}

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
	unsigned int nThreadID;

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

	//std::cout << "AcceptThread Started...Keeping Listenning..." << std::endl;

	pClient->InitialiazeConnection();
	//std::cout << "ConnectionThread End..." << std::endl;

	SAFE_RELEASE(pParam);
	return 0;
}

unsigned int __stdcall CClient::SendThread(LPVOID lpParam)
{
	WORKER_THREAD_PARAM* pParam = (WORKER_THREAD_PARAM*)lpParam;
	CClient* pClient = (CClient*)pParam->pClient;

	//EnterCriticalSection(&pClient->m_csSend);
	auto nBytesSend1 = 0;
	auto nBytesSend2 = 0;

#if 0
	char szSend1[MAX_DATA_BUF_SIZE];
	memset(szSend1, 0, sizeof(szSend1));

	sprintf(szSend1, "%s", /*pParam->szBuffer*/"Hello Server!");
	printf("%s\n", szSend1);
	nBytesSend1 = ::send(pParam->sSocket, "Hello Server!", strlen(szSend1), 0);
	if (SOCKET_ERROR == nBytesSend1)
	{
		std::cout << "Send Message Failed..." << std::endl;
		return 1;
	}
	//std::cout << "Send First Data to Server Successful..." << std::endl;

	Sleep(1000);

	char szSend2[MAX_DATA_BUF_SIZE];
	memset(szSend2, 0, sizeof(szSend2));
	sprintf(szSend2, "%s", "Hello Server!See you Again!");
	printf("%s\n", szSend2);
	nBytesSend2 = ::send(pParam->sSocket, "Hello Server!See you Again!", strlen(szSend2), 0);
	if (SOCKET_ERROR == nBytesSend2)
	{
		std::cout << "Send Message Failed..." << std::endl;
		return 1;
	}
	//std::cout << "Send Second Data to Server Successful..." << std::endl;

#elif 0
	BASE_DATA stuSendData = { 0 };
	stuSendData.stuRequestHead.nRequest = MAC_TEST_SEND;
	strcpy(stuSendData.dataBuff, "Hello Server!");

	//std::cout << (char*)&stuSendData << std::endl;
	//std::cout << sizeof(stuSendData) << std::endl;

	nBytesSend2 = ::send(pParam->sSocket, (char*)&stuSendData, sizeof(stuSendData), 0);
	if (SOCKET_ERROR == nBytesSend2)
	{
		std::cout << "Send Message Failed..." << std::endl;
		return 1;
	}
	std::cout << "Send Data to Server Successful..." << std::endl;

	/*memset(&stuSendData, 0, sizeof(stuSendData));
	stuSendData.stuRequestHead.nRequest = MAC_TEST_SEND_EX;
	strcpy(stuSendData.dataBuff, "Hello Server! See you Again!");

	nBytesSend = ::send(pParam->sSocket, (char*)&stuSendData, sizeof(stuSendData), 0);
	if (SOCKET_ERROR == nBytesSend)
	{
	std::cout << "Send Message Failed..." << std::endl;
	return 1;
	}
	std::cout << "Send Data to Server Successful..." << std::endl;*/
#else

#endif
	//LeaveCriticalSection(&pClient->m_csSend);

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

void CClient::SendData(SOCKET socket, char* pData, int nDataLen)
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
	CClient* pClient = new CClient();
	pClient->Run();

	getchar();
	return 0;
}