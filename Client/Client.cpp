// Client.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

const UINT32 table[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
	0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
	0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
	0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
	0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
	0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
	0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
	0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
	0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
	0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
	0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
	0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
	0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
	0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
	0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
	0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
	0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
	0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
	0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
	0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d,
};

UINT32 getCRC(char* buf, int nLength)
{
	if (nLength < 1)
		return 0xffffffff;

	UINT32 crc = 0;

	for (int i(0); i != nLength; ++i)
	{
		crc = table[(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
	}

	crc = crc ^ 0xffffffff;

	return crc;
}

CClient::CClient(char* szServerIP, char* szClientIP, int nPort, int nThreads)
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
	std::cout << "socket:" << *pSocket << std::endl;

	return TRUE;
}

void CClient::TestIocpConnect()
{
	WSAData data;
	LPFN_CONNECTEX ConnectEx;
	LPFN_DISCONNECTEX DisconnectEx;
	::WSAStartup(0x0202, &data);

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(8888);
	SOCKET sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	DWORD dwSend, dwBytes;
	char szBuffer[] = "hello world";
	OVERLAPPED ol;
	memset(&ol, 0, sizeof(ol));
	GUID guidConnectEx = WSAID_CONNECTEX;
	GUID guidDisconnectEx = WSAID_DISCONNECTEX;
	if (SOCKET_ERROR == WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidConnectEx, sizeof(guidConnectEx), &ConnectEx, sizeof(ConnectEx), &dwBytes, NULL, NULL))
	{
		printf("得到扩展函数指针失败!\r\n");
		getchar();
	}
	if (SOCKET_ERROR == WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidDisconnectEx, sizeof(guidDisconnectEx), &DisconnectEx, sizeof(DisconnectEx), &dwBytes, NULL, NULL))
	{
		printf("得到扩展函数指针失败\r\n");
		getchar();
	}
	SOCKADDR_IN local;
	local.sin_family = AF_INET;
	local.sin_addr.S_un.S_addr = INADDR_ANY;
	local.sin_port = 0;
	if (SOCKET_ERROR == bind(sock, (LPSOCKADDR)&local, sizeof(local)))
	{
		printf("绑定套接字失败!\r\n");
		getchar();
	}

	//连接服务器
	if (!ConnectEx(sock, (const sockaddr*)&addr, sizeof(addr), NULL, 0, &dwSend, &ol))
	{
		DWORD dwError = WSAGetLastError();
		if (ERROR_IO_PENDING != dwError)
		{
			printf("连接服务器失败\r\n");
			getchar();
		}
	}
	DWORD dwFlag = 0, dwTrans;
	if (!WSAGetOverlappedResult(sock, &ol, &dwTrans, TRUE, &dwFlag))
	{
		printf("等待异步结果失败\r\n");
		getchar();
	}

	PACKET_HEAD stuHead = { 0 };
	std::string str = "helloworld";
	stuHead.uiPacketNo = sizeof(PACKET_HEAD) + str.size() - sizeof(int);
	stuHead.uiMsgType = 1;
	stuHead.uiPacketLen = str.size();

	CBufferEx myDataPool;
	myDataPool.Write((PBYTE)&stuHead, sizeof(PACKET_HEAD));
	myDataPool.Write((PBYTE)str.c_str(), str.size());

	WSABUF buf = {0};
	buf.buf = (CHAR*)myDataPool.c_Bytes();
	buf.len = myDataPool.GetLength();
	//发送数据
	if (WSASend(sock, &buf, 1, &dwSend, dwFlag, &ol, NULL))
	{
		DWORD dwError = WSAGetLastError();
		if (ERROR_IO_PENDING != dwError)
		{
			printf("发送失败\r\n");
			getchar();
		}

	}
	if (!WSAGetOverlappedResult(sock, &ol, &dwTrans, TRUE, &dwFlag))
	{
		printf("等待异步结果失败\r\n");
		getchar();
	}

	////断开连接
	//if (!DisconnectEx(sock, &ol, 0, 0)) {
	//	DWORD dwError = WSAGetLastError();
	//	if (ERROR_IO_PENDING != dwError)
	//	{
	//		printf("断开服务器失败\r\n");
	//		getchar();
	//	}
	//}
	//if (!WSAGetOverlappedResult(sock, &ol, &dwTrans, TRUE, &dwFlag))
	//{
	//	printf("等待异步结果失败\r\n");
	//	getchar();
	//}

	//closesocket(sock);
	//WSACleanup();
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

	SAFE_DELETE(m_phSendThreads);
	SAFE_DELETE(m_phRecvThreads);

	SAFE_RELEASE_HANDLE(m_hConnectionThread);

	SAFE_DELETE(m_pWorkerThreadParam);

	SAFE_RELEASE_HANDLE(m_hShutdownEvent);
}

unsigned int __stdcall CClient::ConnectionThread(LPVOID lpParam)
{
	WORKER_THREAD_PARAM* pParam = (WORKER_THREAD_PARAM*)lpParam;
	CClient* pClient = (CClient*)pParam->pClient;
	pClient->InitialiazeConnection();

	SAFE_DELETE(pParam);
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
	PACKET_HEAD stuHead = { 0 };
	std::string str = "helloworld";
	stuHead.uiPacketNo = sizeof(PACKET_HEAD) + str.size() - sizeof(int);
	stuHead.uiMsgType = 1;
	stuHead.uiPacketLen = str.size();

	CBufferEx myDataPool;
	myDataPool.Write((PBYTE)&stuHead, sizeof(PACKET_HEAD));
	myDataPool.Write((PBYTE)str.c_str(), str.size());

	SendData(socket, (char*)myDataPool.c_Bytes(), myDataPool.GetLength());
	std::cout << "SendData Size:" << myDataPool.GetLength() << "bytes" << std::endl;
}

int _tmain(int argc, _TCHAR* argv[])
{
#if 0
	for (auto i = 0; i < 1; i++)
	{
		CClient* pClient = new CClient("127.0.0.1", "127.0.0.1", 8888, 1);
		pClient->Run();
	}
#elif 1
	CIocpClient IocpClient = CIocpClient();
	if (IocpClient.Create("127.0.0.1", 8888, 10, 20, 10000, 4, 0, 1))
	{
		PACKET_HEAD stuHead = { 0 };
		std::string str = "hello";
		stuHead.uiPacketNo = sizeof(PACKET_HEAD) + str.size() - sizeof(int);
		stuHead.uiMsgType = 1;
		stuHead.uiPacketLen = str.size();

		CONTEXT_HEAD lpHead = { 0 };
		REQUEST lpRequest;

		CBuffer myDataPool;
		myDataPool.Write((PBYTE)& stuHead, sizeof(PACKET_HEAD));
		myDataPool.Write((PBYTE)str.c_str(), str.size());

		lpRequest.m_pDataPtr = myDataPool.GetBuffer();
		lpRequest.m_nDataLen = myDataPool.GetBufferLen();

		SOCKET hSocket = IocpClient.GetSocket();
		IocpClient.SendBufferData("hello", 6);
}
	getchar();
	IocpClient.Destroy();
#else
#endif
	getchar();
	return 0;
}