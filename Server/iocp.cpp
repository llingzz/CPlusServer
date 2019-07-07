#include "stdafx.h"

/*CIocpWorker*/
CIocpWorker::CIocpWorker()
{

}

CIocpWorker::~CIocpWorker()
{

}

BOOL CIocpWorker::BeginWorkerPool(int nThreads, int nConcurrency)
{
	for (int i = 0; i < nThreads; i++)
	{
		UINT uiThreadID = 0;
		HANDLE hHandle = (HANDLE)::_beginthreadex(nullptr, 0, WorkerThreadFunc, (void*)this, 0, &uiThreadID);
		if (INVALID_HANDLE_VALUE != hHandle)
		{
			CAutoLock lock(&m_csWorkers);
			m_mapWorkers.insert(std::make_pair(uiThreadID, hHandle));
		}
	}

	return TRUE;
}

BOOL CIocpWorker::EndWorkerPool()
{
	ClearWorkerOvLists();
	CloseWorkerHandles();
	return TRUE;
}

BOOL CIocpWorker::DoWorkLoop()
{
	while (TRUE)
	{
		WORKER_OV* pWorkerOv = nullptr;
		WORKER_OV* pFreeWorkerOv = MoveWorkerOvToFree(pWorkerOv);
		if (pWorkerOv)
		{
			OnRequest(pWorkerOv->m_p1, pWorkerOv->m_p2);
		}
		if (pFreeWorkerOv)
		{
			SAFE_DELETE(pFreeWorkerOv->m_p1);
			SAFE_DELETE(pFreeWorkerOv->m_p2);
			SAFE_DELETE(pFreeWorkerOv);
		}
	}
	return FALSE;
}

BOOL CIocpWorker::PutRequestToQueue(DWORD dwSize, DWORD dwKey, void* pParam1, void* pParam2)
{
	WORKER_OV* pWorkerOv = AllocateWorkerOv(pParam1, pParam2);
	return PutDataToQueue(dwSize, dwKey, pWorkerOv);
}

BOOL CIocpWorker::PutDataToQueue(DWORD dwSize, DWORD dwKey, WORKER_OV* pWorkerOv)
{
	CAutoLock lock(&m_csWorkerOvList);
	pWorkerOv->m_ol.hEvent = (HANDLE)dwKey;
	pWorkerOv->m_ol.Offset = dwSize;
	m_listWorkerOv.push_back(pWorkerOv);
	return TRUE;
}

BOOL CIocpWorker::OnRequest(void* pParam1, void* pParam2)
{
	return TRUE;
}

WORKER_OV* CIocpWorker::AllocateWorkerOv(void* pParam1, void* pParam2)
{
	WORKER_OV* pWorkerOv = new WORKER_OV(pParam1, pParam2);
	return pWorkerOv;
}

WORKER_OV* CIocpWorker::MoveWorkerOvToFree(WORKER_OV* pWorkerOv)
{
	CAutoLock lock(&m_csWorkerOvList);
	if (m_listWorkerOv.size() > 0)
	{
		pWorkerOv = (WORKER_OV*)m_listWorkerOv.front();
		m_listWorkerOv.pop_front();
		m_nWorkerOvCount--;
	}
	if (nullptr != pWorkerOv)
	{
		m_listFreeWorkerOv.push_back(pWorkerOv);
	}
	WORKER_OV* pFreeWorkOv = nullptr;
	if (m_listFreeWorkerOv.size() > 0)
	{
		pFreeWorkOv = (WORKER_OV*)m_listFreeWorkerOv.front();
		m_listFreeWorkerOv.pop_front();
	}
	else
	{
		m_nFreeWorkerOvCount++;
	}
	return pFreeWorkOv;
}

UINT CIocpWorker::GetWorkerCount()
{
	CAutoLock lock(&m_csWorkers);
	return m_mapWorkers.size();
}

void CIocpWorker::ClearWorkerOvLists()
{
	CAutoLock lock(&m_csWorkerOvList);
	while (m_listWorkerOv.size() > 0)
	{
		WORKER_OV* pWorkerOv = (WORKER_OV*)m_listWorkerOv.front();
		SAFE_DELETE(pWorkerOv);
		m_listWorkerOv.pop_front();
	}
	while (m_listFreeWorkerOv.size() > 0)
	{
		WORKER_OV* pWorkerOv = (WORKER_OV*)m_listFreeWorkerOv.front();
		SAFE_DELETE(pWorkerOv);
		m_listFreeWorkerOv.pop_front();
	}
}

void CIocpWorker::CloseWorkerHandles()
{
	CAutoLock lock(&m_csWorkers);
	std::map<UINT, LPVOID>::iterator iter = m_mapWorkers.begin();
	for (; iter != m_mapWorkers.end(); iter++)
	{
		if (INVALID_HANDLE_VALUE != iter->second)
		{
			::CloseHandle(iter->second);
		}
	}
}

unsigned __stdcall CIocpWorker::WorkerThreadFunc(LPVOID lpParam)
{
	CIocpWorker* pWorker = (CIocpWorker*)lpParam;
	if (pWorker)
	{
		pWorker->DoWorkLoop();
	}
	myLogConsoleI("%s �߳�%d�˳�...", __FUNCTION__, GetCurrentThreadId());
	return THREAD_EXIT;
}

/*CIocpServer*/
CIocpServer::CIocpServer()
{

}

CIocpServer::~CIocpServer()
{

}

BOOL CIocpServer::InitializeMembers()
{
	m_pSocketBufferList = NULL;
	m_pSocketContextList = NULL;
	m_pConnectionContext = NULL;

	memset(m_hWaitHandle, 0, 2 * sizeof(HANDLE));
	memset(m_hWorkerHandle, 0, MAX_WORKER_THREAD_NUMBER * sizeof(HANDLE));

	m_nSocketBufferListCount = 0;
	m_nMaxSocketBufferListCount = 0;

	m_nSocketContextListCount = 0;
	m_nMaxSocketContextListCount = 0;

	m_nConnectionContextCount = 0;
	m_nMaxConnections = 0;

	m_pListenContext = NULL;
	m_nMaxAccepts = 0;

	memset(m_szIp, 0, IP_LENGTH);
	m_nPort = 0;

	m_uiAcceptThread = 0;
	m_nMaxSendCount = 0;
	m_nWorkerThreads = 0;
	m_nConcurrency = 0;

	m_bShutdown = FALSE;

	return TRUE;
}

BOOL CIocpServer::Shutdown()
{
	m_bShutdown = TRUE;

	ClearConnectionContext();

	for (int i = 0; i < m_nWorkerThreads; i++)
	{
		::PostQueuedCompletionStatus(m_hCompletionPort, 0, 0, NULL);
	}
	::WaitForMultipleObjects(m_nWorkerThreads, m_hWorkerHandle, TRUE, 1000 * m_nWorkerThreads);
	for (int i = 0; i < m_nWorkerThreads; i++)
	{
		SAFE_RELEASE_HANDLE(m_hWorkerHandle[i]);
	}

	FreeSocketBuffer();
	FreeSocketContext();

	if (m_pListenContext)
	{
		SAFE_RELEASE_HANDLE(m_pListenContext->m_hAcceptHandle);
		SAFE_RELEASE_HANDLE(m_pListenContext->m_hRepostHandle);
		SAFE_RELEASE_SOCKET(m_pListenContext->m_hSocket);
		SAFE_DELETE(m_pListenContext);
	}

	SAFE_RELEASE_HANDLE(m_hCompletionPort);

	myLogConsoleI("%s ����ر�", __FUNCTION__);
	//EndWorkerPool();
	WSACleanup();
	return TRUE;
}

BOOL CIocpServer::OnRequest(void* lpParam1, void* lpParam2)
{
	LPCONTEXT_HEAD lpContext = (LPCONTEXT_HEAD)lpParam1;
	LPREQUEST lpRequest = (LPREQUEST)lpParam2;

	switch (lpRequest->m_stHead.nRequest)
	{
	default:
		break;
	}

	SAFE_DELETE(lpRequest->m_pDataPtr);
	return TRUE;
}

BOOL CIocpServer::BeginListen(const char* lpSzIp, UINT nPort, UINT nInitAccepts, UINT nMaxAccpets)
{
	m_nPort = nPort;
	lstrcpy(m_szIp, (TCHAR*)lpSzIp);

	m_pListenContext = new CSocketListenContext;
	m_pListenContext->m_hAcceptHandle = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	m_pListenContext->m_hRepostHandle = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	m_pListenContext->m_nInitAccpets = nInitAccepts;
	m_pListenContext->m_hSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	myLogConsoleI("%s �����׽���%d��ʼ��", __FUNCTION__, m_pListenContext->m_hSocket);
	SOCKADDR_IN sock_in;
	memset(&sock_in, 0, sizeof(SOCKADDR_IN));
	sock_in.sin_family = AF_INET;
	sock_in.sin_port = ::ntohs(m_nPort);
	sock_in.sin_addr.S_un.S_addr = inet_addr((char*)m_szIp);;
	::bind(m_pListenContext->m_hSocket, (SOCKADDR*)&sock_in, sizeof(sock_in));
	::listen(m_pListenContext->m_hSocket, SOMAXCONN);

	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	DWORD dwBytes;
	::WSAIoctl(m_pListenContext->m_hSocket,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidAcceptEx,
		sizeof(GuidAcceptEx),
		&(m_pListenContext->m_lpfnAcceptEx),
		sizeof(m_pListenContext->m_lpfnAcceptEx),
		&dwBytes,
		NULL,
		NULL);
	GUID GuidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
	::WSAIoctl(m_pListenContext->m_hSocket,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidGetAcceptExSockaddrs,
		sizeof(GuidGetAcceptExSockaddrs),
		&(m_pListenContext->m_lpfnGetAcceptExSockaddrs),
		sizeof(m_pListenContext->m_lpfnGetAcceptExSockaddrs),
		&dwBytes,
		NULL,
		NULL);
	::CreateIoCompletionPort((HANDLE)m_pListenContext->m_hSocket, m_hCompletionPort, (DWORD)0, 0);
	::WSAEventSelect(m_pListenContext->m_hSocket, m_pListenContext->m_hAcceptHandle, FD_ACCEPT);
	m_hAcceptThread = (HANDLE)::_beginthreadex(NULL, 0, AcceptThreadFunc, (void*)this, 0, &m_uiAcceptThread);

	return TRUE;
}

BOOL CIocpServer::BeginThreadPool(UINT nThreads, UINT nConcurrency)
{
	m_nWorkerThreads = (nThreads <= MAX_WORKER_THREAD_NUMBER) ? nThreads : MAX_WORKER_THREAD_NUMBER;
	m_nConcurrency = nConcurrency;

	// ���������̣߳��Ƽ��߳�������������*2
	for (int i = 0; i < m_nWorkerThreads; i++)
	{
		unsigned int uiWorkerThread = 0;
		m_hWorkerHandle[i] = (HANDLE)::_beginthreadex(NULL, 0, WorkerThreadFunc, (void*)this, 0, &uiWorkerThread);
		if (INVALID_HANDLE_VALUE == m_hWorkerHandle[i])
		{
			myLogConsoleE("�����̴߳���ʧ��...");
			return FALSE;
		}
	}

	return TRUE;
}

BOOL CIocpServer::InitializeIo()
{
	WSADATA wsaData;
	::WSAStartup(MAKEWORD(2, 2), &wsaData);

	m_hCompletionPort = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, m_nConcurrency);

	return TRUE;
}

BOOL CIocpServer::Initialize(const char* lpSzIp, UINT nPort, UINT nInitAccepts, UINT nMaxAccpets, UINT nMaxSocketBufferListCount, UINT nMaxSocketContextListCount, UINT nMaxSendCount, UINT nThreads, UINT nConcurrency, UINT nMaxConnections)
{
	InitializeMembers();
	m_nMaxAccepts = nMaxAccpets;
	m_nMaxSocketBufferListCount = nMaxSocketBufferListCount;
	m_nMaxSocketContextListCount = nMaxSocketContextListCount;
	m_nMaxSendCount = nMaxSendCount;
	m_nMaxConnections = nMaxConnections;

	//BeginWorkerPool(m_nWorkerThreads, m_nConcurrency);
	if (!InitializeIo())
	{
		myLogConsoleW("%s InitializeIoʧ��", __FUNCTION__);
		return FALSE;
	}
	if (!BeginListen(lpSzIp, nPort, nInitAccepts, nMaxAccpets))
	{
		myLogConsoleW("%s BeginListenʧ��", __FUNCTION__);
		return FALSE;
	}
	if (!BeginThreadPool(nThreads, nConcurrency))
	{
		myLogConsoleW("%s BeginThreadPoolʧ��", __FUNCTION__);
		return FALSE;
	}

	this;

	return TRUE;
}

BOOL CIocpServer::PostAccept(CSocketContext* pContext, CSocketBuffer* pBuffer, DWORD& dwWSAError)
{
	pBuffer->m_ioType = enIoAccept;
	DWORD dwBytes = 0;
	pBuffer->m_hSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	BOOL bRet = m_pListenContext->m_lpfnAcceptEx(
		m_pListenContext->m_hSocket,
		pBuffer->m_hSocket,
		pBuffer->m_pBuffer,
		/*����Accept��ʱ��������������ӵĵ�һ������*/
		/*pBuffer->m_nBufferLen - ((sizeof(SOCKADDR_IN) + ACCEPTEX_BYTES_OFFSET_SIZE) * 2)*/0,
		sizeof(SOCKADDR_IN) + ACCEPTEX_BYTES_OFFSET_SIZE,
		sizeof(SOCKADDR_IN) + ACCEPTEX_BYTES_OFFSET_SIZE,
		&dwBytes,
		&(pBuffer->m_ol)
	);
	dwWSAError = ::WSAGetLastError();
	if (!bRet && WSA_IO_PENDING != dwWSAError)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CIocpServer::PostRecv(CSocketContext* pContext, CSocketBuffer* pBuffer, DWORD& dwWSAError)
{
	DWORD dwBytes = 0;
	DWORD dwFlags = 0;
	pBuffer->m_ioType = enIoRead;

	::EnterCriticalSection(&(pContext->m_csLock));
	pBuffer->m_hSocket = pContext->m_hSocket;
	pBuffer->m_nSerialNo = pContext->m_nNextReadSerialNo;
	WSABUF wsaBuf = { 0 };
	wsaBuf.buf = (CHAR*)pBuffer->m_pBuffer;
	wsaBuf.len = pBuffer->m_nBufferLen;
	DWORD dwRet = ::WSARecv(pContext->m_hSocket, &wsaBuf, 1, &dwBytes, &dwFlags, &pBuffer->m_ol, NULL);
	dwWSAError = ::WSAGetLastError();
	if (NO_ERROR != dwRet && WSA_IO_PENDING != dwWSAError)
	{
		::LeaveCriticalSection(&(pContext->m_csLock));
		return FALSE;
	}
	pContext->m_nPostedRecv++;
	pContext->m_nNextReadSerialNo++;
	::LeaveCriticalSection(&(pContext->m_csLock));

	return TRUE;
}

BOOL CIocpServer::PostSend(CSocketContext* pContext, CSocketBuffer* pBuffer, DWORD& dwWSAError)
{
	::EnterCriticalSection(&(pContext->m_csLock));
	if (pContext->m_nPostedSend > m_nMaxSendCount)
	{
		::LeaveCriticalSection(&(pContext->m_csLock));
		return FALSE;
	}

	DWORD dwBytes = 0;
	DWORD dwFlags = 0;
	pBuffer->m_ioType = enIoWrite;
	pBuffer->m_hSocket = pContext->m_hSocket;

	WSABUF wsaBuf = { 0 };
	wsaBuf.buf = (CHAR*)pBuffer->m_pBuffer;
	wsaBuf.len = pBuffer->m_nBufferLen;

	DWORD dwRet = ::WSASend(pContext->m_hSocket, &wsaBuf, 1, &dwBytes, dwFlags, &pBuffer->m_ol, NULL);
	dwWSAError = ::WSAGetLastError();
	if (NO_ERROR != dwRet && WSA_IO_PENDING != dwWSAError)
	{
		::LeaveCriticalSection(&(pContext->m_csLock));
		return FALSE;
	}
	pContext->m_nPostedSend++;
	::LeaveCriticalSection(&(pContext->m_csLock));

	return TRUE;
}

BOOL CIocpServer::OnReceiveData(CSocketContext* pContext, CSocketBuffer* pBuffer)
{
	/*�յ����ݰ��������ݷ��뻺����*/
	UINT uiPacketHeadLen = sizeof(PACKET_HEAD);
	::EnterCriticalSection(&(pContext->m_csLock));
	UINT uiDataBufLen = pContext->m_objDataBuf.GetBufferLen();
	if (pBuffer->m_nBufferLen > 0)
	{
		pContext->m_objDataBuf.Write(pBuffer->m_pBuffer, pBuffer->m_nBufferLen);
		uiDataBufLen += pBuffer->m_nBufferLen;
		pContext->m_nPacketNo++;
	}
	while (uiDataBufLen > uiPacketHeadLen)
	{
		/*���ȴ��ڰ�ͷ���ȣ�ȡ�ð�ͷ����*/
		LPPACKET_HEAD lpPacketHead = (LPPACKET_HEAD)(PBYTE(pContext->m_objDataBuf.GetBuffer()));
		if (lpPacketHead && lpPacketHead->uiPacketLen <= (uiDataBufLen - uiPacketHeadLen))
		{
			/*���ݰ�����У�飬У��ʧ�ܣ�����������ر��׽���*/
			BOOL bVerify = OnVerifyData(pContext, pBuffer);
			if(!bVerify)
			{
				myLogConsoleW("%s �������ݰ�У��ʧ�ܣ��׽��ֶϿ�", __FUNCTION__);
				CloseConnectionContext(pContext);
				if (0 == pContext->m_nPostedRecv && 0 == pContext->m_nPostedSend)
				{
					ReleaseSocketContext(pContext);
				}
				ReleaseSocketBuffer(pBuffer);
				break;
			}

			/*���ݰ�����*/
			OnHandleData(pContext, pBuffer);

			/*������ʣ�µ����ݳ��ȴ��ڻ���ڰ�ͷ��ָ�İ��峤�ȣ�ȡ���������ݰ�*/
			pContext->m_nSessionID++;
		}
		else
		{
			/*������ʣ�����ݳ���С�ڰ�ͷ��ָ�İ��峤�ȣ��޷�ȡ���������ݰ����˳�*/
			break;
		}
		/*��ȡ��ǰ�׽����»��������ݴ�С*/
		uiDataBufLen = pContext->m_objDataBuf.GetBufferLen();
		if (uiDataBufLen > 0)
		{
			myLogConsoleI("%s ��ǰ�׽����»��������ݴ�Сʣ�ࣺ%d���ֽڣ�", __FUNCTION__, uiDataBufLen);
			myLogConsoleI("%s ��ǰ�׽������������ݻ��������ݴ�Сʣ�ࣺ%d���ֽڣ�", __FUNCTION__, pContext->m_objReqBuf.GetBufferLen());
			myLogConsoleI("%s ��ǰ�׽����»ظ����ݻ��������ݴ�Сʣ�ࣺ%d���ֽڣ�", __FUNCTION__, pContext->m_objResBuf.GetBufferLen());
		}
	}
	::LeaveCriticalSection(&(pContext->m_csLock));
	return TRUE;
}

BOOL CIocpServer::OnVerifyData(CSocketContext* pContext, CSocketBuffer* pBuffer)
{
	/*���ݰ�У�飨CRC32����У��ʧ�ܣ�����������ر��׽���sockte����*/
	LPPACKET_HEAD lpHead = (LPPACKET_HEAD)(PBYTE(pContext->m_objDataBuf.GetBuffer()));
	/*���ݰ�����У�飨�߽�У��/����СУ��/���ݰ�����У�飩...*/
	if (!lpHead)
	{
		myLogConsoleW("%s lpHeadΪ��", __FUNCTION__);
		return FALSE;
	}
	if (lpHead->uiPacketLen > BASE_DATA_BUF_SIZE || lpHead->uiPacketLen <= 0)
	{
		myLogConsoleW("%s ���ݰ��ĳ���%dУ��ʧ��", __FUNCTION__, lpHead->uiPacketLen);
		return FALSE;
	}
	if (lpHead->bUseCRC32)
	{
		/*Ҫ��У��CRC32��ȷ���շ������Ƿ�һ��*/
	}
	return TRUE;
}

BOOL CIocpServer::OnHandleData(CSocketContext* pContext, CSocketBuffer* pBuffer)
{
	/*���ݰ�����*/
	LPPACKET_HEAD lpHead = (LPPACKET_HEAD)(PBYTE(pContext->m_objDataBuf.GetBuffer()));
	UINT uiMsgType = lpHead->uiMsgType;
	UINT uiFullDataLen = sizeof(PACKET_HEAD) + lpHead->uiPacketLen;
	PBYTE pFullData = new BYTE[uiFullDataLen];
	pContext->m_objDataBuf.Read(pFullData, uiFullDataLen);
	if (enRequest == uiMsgType)
	{
		pContext->m_objReqBuf.Write((PBYTE)(pFullData + sizeof(PACKET_HEAD)), uiFullDataLen - sizeof(PACKET_HEAD));
	}
	else if (enResponse == uiMsgType)
	{
		pContext->m_objResBuf.Write((PBYTE)(pFullData + sizeof(PACKET_HEAD)), uiFullDataLen - sizeof(PACKET_HEAD));
	}
	else
	{
		/*�ݲ�����*/
		myLogConsoleI("%s δ֪��Ϣ����%d", __FUNCTION__, uiMsgType);
	}
	SAFE_DELETE_ARRAY(pFullData);

	return TRUE;
}

BOOL CIocpServer::SendData(SOCKET hSocket, LPCONTEXT_HEAD lpContextHead, LPREQUEST lpRequest, UINT uiMsgType)
{
	CBuffer myDataPool;
	ComposePacket(myDataPool, uiMsgType, lpRequest->m_pDataPtr, sizeof(REQUEST));

	CSocketBuffer* pBuffer = AllocateSocketBuffer(myDataPool.GetBufferLen());
	if (pBuffer)
	{
		memcpy(pBuffer->m_pBuffer, myDataPool.GetBuffer(), myDataPool.GetBufferLen());
		DWORD dwError = 0;
		CSocketContext* pContext = FindClient(hSocket);
		if (pContext)
		{
			return PostSend(pContext, pBuffer, dwError);
		}
		else
		{
			myLogConsoleW("%s û���ҵ���Ӧ����%d���׽�����������Ϣ", __FUNCTION__, hSocket);
			return FALSE;
		}
	}

	return FALSE;
}

void CIocpServer::ComposePacket(CBuffer& dstBuf, UINT nMsgType, LPVOID pData, UINT nPacketSize)
{
	PACKET_HEAD stuHead = { 0 };
	stuHead.uiMsgType = nMsgType;
	stuHead.uiPacketLen = nPacketSize;

	dstBuf.Write((PBYTE)&stuHead, sizeof(PACKET_HEAD));
	dstBuf.Write((PBYTE)pData, nPacketSize);
}

void CIocpServer::DecomposePacket(CBuffer& srcBuf, CBuffer& dstBuf, int nPacketSize)
{

}

void CIocpServer::HandleIo(DWORD dwKey, CSocketBuffer* pBuffer, DWORD dwTrans, DWORD dwError)
{
	CSocketContext* pContext = (CSocketContext*)dwKey;
	if (pContext)
	{
		/*���¸��׽����ϴ�����I/O��������*/
		::EnterCriticalSection(&(pContext->m_csLock));
		if (enIoWrite == pBuffer->m_ioType)
		{
			pContext->m_nPostedSend--;
		}
		else if (enIoRead == pBuffer->m_ioType)
		{
			pContext->m_nPostedRecv--;
		}
		::LeaveCriticalSection(&(pContext->m_csLock));
		/*�����׽����Ƿ��Ѿ����رգ��ر�����Ҫ�ͷŸ��׽����ϴ�������I/O����*/
		if (pContext->m_bClose)
		{
			myLogConsoleW("%s �׽���%d�Ѿ����ر�", __FUNCTION__, pContext->m_hSocket);
			if (0 == pContext->m_nPostedRecv && 0 == pContext->m_nPostedSend)
			{
				ReleaseSocketContext(pContext);
			}
			ReleaseSocketBuffer(pBuffer);
			return;
		}
	}
	else
	{
		/*���׽������ӽ���ʱ��dwKey = 0 dwTrans = 0 dwError = 0����ʱpContext��ΪNULL�����´���������������*/
		if (enIoAccept == pBuffer->m_ioType)
		{
			RemovePendingAccepts(pBuffer);
		}
	}
	/*�����׽������Ƿ�������*/
	if (NO_ERROR != dwError)
	{
		if (enIoAccept != pBuffer->m_ioType)
		{
			/*�׽��ַ������󣬶Ͽ����ӣ��ͷ������Դ*/
			myLogConsoleW("%s �׽���%d��������", __FUNCTION__, pBuffer->m_hSocket);
			CloseConnectionContext(pContext);
			if (0 == pContext->m_nPostedRecv && 0 == pContext->m_nPostedSend)
			{
				ReleaseSocketContext(pContext);
			}
		}
		else
		{
			/*�������׽����Ϸ������󣬷���������رռ����ĸ�socket�׽���*/
			if (INVALID_SOCKET != pBuffer->m_hSocket)
			{
				SAFE_RELEASE_SOCKET(pBuffer->m_hSocket);
			}
		}
	}
	/*�ֱ���I/O����*/
	switch (pBuffer->m_ioType)
	{
	case enIoAccept:
		HandleIoAccept(dwKey, pBuffer, dwTrans, dwError);
		break;
	case enIoRead:
		HandleIoRead(dwKey, pBuffer, dwTrans, dwError);
		break;
	case enIoWrite:
		HandleIoWrite(dwKey, pBuffer, dwTrans, dwError);
		break;
	default:
		HandleIoDefault(dwKey, pBuffer, dwTrans, dwError);
		break;
	}
}

void CIocpServer::HandleIoDefault(DWORD dwKey, CSocketBuffer* pBuffer, DWORD dwTrans, DWORD dwError)
{
	// ��ʱ��������
}

void CIocpServer::HandleIoAccept(DWORD dwKey, CSocketBuffer* pBuffer, DWORD dwTrans, DWORD dwError)
{
	/*����Accept����*/
	CSocketContext* pContext = AllocateSocketContext(pBuffer->m_hSocket);
	if (pContext)
	{
		BOOL bRet = InsertConnectionContext(pContext);
		if (bRet)
		{
			LPSOCKADDR lpLocalAddr = NULL;
			LPSOCKADDR lpRemoteAddr = NULL;
			int nLocalLen = sizeof(SOCKADDR_IN);
			int nRemoteLen = sizeof(SOCKADDR_IN);

			m_pListenContext->m_lpfnGetAcceptExSockaddrs(
				pBuffer->m_pBuffer,
				/*����Accept��ʱ��������������ӵĵ�һ������*/
				/*pBuffer->m_nBufferLen - ((sizeof(SOCKADDR_IN) + ACCEPTEX_BYTES_OFFSET_SIZE) * 2)*/0,
				sizeof(SOCKADDR_IN) + ACCEPTEX_BYTES_OFFSET_SIZE,
				sizeof(SOCKADDR_IN) + ACCEPTEX_BYTES_OFFSET_SIZE,
				(LPSOCKADDR*)&lpLocalAddr,
				&nLocalLen,
				(LPSOCKADDR*)&lpRemoteAddr,
				&nRemoteLen);
			memcpy(&pContext->m_local, lpLocalAddr, nLocalLen);
			memcpy(&pContext->m_remote, lpRemoteAddr, nRemoteLen);
			/*���½����ӵ��׽���socket����ɶ˿ڰ�*/
			::CreateIoCompletionPort((HANDLE)pContext->m_hSocket, m_hCompletionPort, (DWORD)pContext, 0);
			myLogConsoleI("%s:���׽���%d���ӽ���", __FUNCTION__, pContext->m_hSocket);
#if 1
			CONTEXT_HEAD lpHead = { 0 };
			REQUEST lpRequest;

			std::string str = "helloworld";
			REQUEST_HEAD stuReqHead = { 0 };

			CBuffer myDataPool;
			myDataPool.Write((PBYTE)&stuReqHead, sizeof(REQUEST_HEAD));
			myDataPool.Write((PBYTE)str.c_str(), str.size());

			lpRequest.m_pDataPtr = myDataPool.GetBuffer();
			lpRequest.m_nDataLen = myDataPool.GetBufferLen();
			SendData(pContext->m_hSocket, &lpHead, &lpRequest, enRequest);
#else
#endif
			/*Ϊ������Ͷ��һ��Read����*/
			CSocketBuffer* pNewBuffer = AllocateSocketBuffer(DATA_BUF_SIZE);
			if (pNewBuffer)
			{
				DWORD dwError = 0;
				if (!PostRecv(pContext, pNewBuffer, dwError))
				{
					/*Ͷ��ʧ�ܣ��ر�������*/
					CloseConnectionContext(pContext);
					myLogConsoleW("%s �������׽���%dͶ��Read����ʧ�ܣ��Ͽ�����", __FUNCTION__, pContext->m_hSocket);
				}
			}
		}
		else
		{
			/*�ﵽ�涨����������ƣ��ر����ӣ��ͷ����������Դ*/
			myLogConsoleW("%s �ﵽ�涨�������������%d���ر�����", __FUNCTION__, m_nMaxConnections);
			CloseConnectionContext(pContext);
			ReleaseSocketContext(pContext);
		}
	}
	else
	{
		/*��Դ����ʧ�ܣ��رո�����*/
		if (INVALID_SOCKET != pBuffer->m_hSocket)
		{
			SAFE_RELEASE_SOCKET(pBuffer->m_hSocket);
		}
	}

	/*Accept I/O������ɣ��ͷ�pBuffer*/
	ReleaseSocketBuffer(pBuffer);
	/*֪ͨAccept�߳��е�m_hRepostHandle�¼�����Ͷ��һ��Accept����*/
	::InterlockedIncrement(&m_pListenContext->m_nRepostCount);
	::SetEvent(m_pListenContext->m_hRepostHandle);
}

void CIocpServer::HandleIoRead(DWORD dwKey, CSocketBuffer* pBuffer, DWORD dwTrans, DWORD dwError)
{
	/*����Read����*/
	CSocketContext* pContext = (CSocketContext*)dwKey;
	if (pContext)
	{
		if (0 == dwTrans)
		{
			/*�׽��ֶϿ����ͷ������Դ*/
			myLogConsoleW("%s �׽��ֶϿ�", __FUNCTION__);
			CloseConnectionContext(pContext);
			if (0 == pContext->m_nPostedRecv && 0 == pContext->m_nPostedSend)
			{
				ReleaseSocketContext(pContext);
			}
			ReleaseSocketBuffer(pBuffer);
		}
		else
		{
			pBuffer->m_nBufferLen = dwTrans;
			CSocketBuffer* pHandleBuffer = GetRecvSocketBuffer(pContext, pBuffer);
			while (pHandleBuffer)
			{
				/*�ƶ������Ϊ����һ��CSocketBuffer�� �ͷ���ɵ�CSocketBuffer*/
				::InterlockedIncrement(&pContext->m_nCurrentReadSerialNo);
				OnReceiveData(pContext, pBuffer);
				myLogConsoleI("%s:��", __FUNCTION__);
				ReleaseSocketBuffer(pHandleBuffer);
				pHandleBuffer = GetRecvSocketBuffer(pContext, NULL);
			}

			/*Ͷ��һ���µ�Read��Ϣ�� Ͷ��ʧ�ܹر�����*/
			CSocketBuffer* pNewBuffer = AllocateSocketBuffer(DATA_BUF_SIZE);
			DWORD dwError = 0;
			if (!pNewBuffer || !PostRecv(pContext, pNewBuffer, dwError))
			{
				CloseConnectionContext(pContext);
				if (0 == pContext->m_nPostedRecv && 0 == pContext->m_nPostedSend)
				{
					ReleaseSocketContext(pContext);
				}
				ReleaseSocketBuffer(pNewBuffer);
			}
		}
	}
	else
	{
		myLogConsoleW("%s pContextΪ��!!!", __FUNCTION__);
		ReleaseSocketBuffer(pBuffer);
	}
}

void CIocpServer::HandleIoWrite(DWORD dwKey, CSocketBuffer* pBuffer, DWORD dwTrans, DWORD dwError)
{
	/*����Write����*/
	CSocketContext* pContext = (CSocketContext*)dwKey;
	if (0 == dwTrans)
	{
		/*I/Oд���������쳣���رո��׽��ֵ����ӣ��ͷ������Դ*/
		myLogConsoleI("%s I/Oд���������쳣���Է��׽��ֶϿ�", __FUNCTION__);
		CloseConnectionContext(pContext);
		if (0 == pContext->m_nPostedRecv && 0 == pContext->m_nPostedSend)
		{
			ReleaseSocketContext(pContext);
		}
		ReleaseSocketBuffer(pBuffer);
	}
	else
	{
		CSocketBuffer* pSendBuffer = GetSendSocketBuffer(pContext, pBuffer);
		if (pSendBuffer)
		{
			pSendBuffer->m_nBufferLen = dwTrans;
			myLogConsoleI("%s д", __FUNCTION__);
			/*I/Oд������ɣ��ͷ�CSocketBuffer*/
			ReleaseSocketBuffer(pSendBuffer);

			if (pContext->m_nPostedSend > 0)
			{
				/*Ͷ��һ���µ�Send��Ϣ�� Ͷ��ʧ�ܹر�����*/
				CSocketBuffer* pNewBuffer = AllocateSocketBuffer(DATA_BUF_SIZE);
				DWORD dwError = 0;
				if (!pNewBuffer || !PostRecv(pContext, pNewBuffer, dwError))
				{
					CloseConnectionContext(pContext);
					if (0 == pContext->m_nPostedRecv && 0 == pContext->m_nPostedSend)
					{
						ReleaseSocketContext(pContext);
					}
					ReleaseSocketBuffer(pNewBuffer);
				}
			}
		}
	}
}

BOOL CIocpServer::InsertPendingAccepts(CSocketContext* pContext, CSocketBuffer* pBuffer)
{
	::EnterCriticalSection(&(m_pListenContext->m_csLock));
	if (!m_pListenContext->m_pAcceptPendingList)
	{
		m_pListenContext->m_pAcceptPendingList = pBuffer;
	}
	else
	{
		pBuffer->m_pNext = m_pListenContext->m_pAcceptPendingList;
		m_pListenContext->m_pAcceptPendingList = pBuffer;
	}
	m_pListenContext->m_nAcceptPendingListCount++;
	::LeaveCriticalSection(&(m_pListenContext->m_csLock));

	return TRUE;
}

BOOL CIocpServer::RemovePendingAccepts(CSocketBuffer* pBuffer)
{
	BOOL bRet = FALSE;

	::EnterCriticalSection(&(m_pListenContext->m_csLock));
	CSocketBuffer* pTemp = m_pListenContext->m_pAcceptPendingList;
	if (pTemp == pBuffer)
	{
		m_pListenContext->m_pAcceptPendingList = pBuffer->m_pNext;
		bRet = TRUE;
	}
	else
	{
		while (pTemp && pTemp->m_pNext != pBuffer)
		{
			pTemp = pTemp->m_pNext;
		}
		if (pTemp)
		{
			pTemp->m_pNext = pBuffer->m_pNext;
			bRet = TRUE;
		}
	}

	if (bRet)
	{
		m_pListenContext->m_nAcceptPendingListCount--;
	}
	::LeaveCriticalSection(&(m_pListenContext->m_csLock));

	return TRUE;
}

BOOL CIocpServer::InsertConnectionContext(CSocketContext* pContext)
{
	/*���������ӵ��׽���������*/
	CAutoLock lock(&m_csConnectionContext);
	if (m_nConnectionContextCount < m_nMaxConnections)
	{
		pContext->m_pNext = m_pConnectionContext;
		m_pConnectionContext = pContext;
		m_nConnectionContextCount++;
		return TRUE;
	}
	else
	{
		myLogConsoleW("��ǰ�Ѿ��ﵽ�׽����������ޣ���ֹ���ӣ�");
		return FALSE;
	}
}

BOOL CIocpServer::CloseConnectionContext(CSocketContext* pContext)
{
	/*�ر�ָ�����׽���������*/
	{
		CAutoLock lock(&m_csConnectionContext);
		CSocketContext* pTemp = m_pConnectionContext;
		if (pTemp == pContext)
		{
			m_pConnectionContext = pContext->m_pNext;
			m_nConnectionContextCount--;
		}
		else
		{
			while (pTemp && pTemp->m_pNext != pContext)
			{
				pTemp = pTemp->m_pNext;
			}
			if (pTemp)
			{
				pTemp->m_pNext = pContext->m_pNext;
				m_nConnectionContextCount--;
			}
		}
	}
	/*�ر�socket�׽���*/
	{
		::EnterCriticalSection(&(pContext->m_csLock));
		SAFE_RELEASE_SOCKET(pContext->m_hSocket);
		pContext->m_bClose = TRUE;
		::LeaveCriticalSection(&(pContext->m_csLock));
	}

	return TRUE;
}

BOOL CIocpServer::ClearConnectionContext()
{
	/*�ͷŵ�ǰ���������׽���������*/
	CAutoLock lock(&m_csConnectionContext);
	CSocketContext* pContext = m_pConnectionContext;
	while (pContext)
	{
		::EnterCriticalSection(&(pContext->m_csLock));
		SAFE_RELEASE_SOCKET(pContext->m_hSocket);
		pContext->m_bClose = TRUE;
		CSocketContext* pTemp = pContext->m_pNext;
		::LeaveCriticalSection(&(pContext->m_csLock));
		pContext = pTemp;
	}

	m_pConnectionContext = NULL;
	m_nConnectionContextCount = 0;

	return TRUE;
}

CSocketBuffer* CIocpServer::AllocateSocketBuffer(UINT nBufferLen)
{
	/*����I/O�����ṹ�������ڿ���I/O�����ṹ�б���ȡ*/
	CSocketBuffer* pBuffer = NULL;
	if (nBufferLen <= 0 || nBufferLen > DATA_BUF_SIZE)
	{
		return NULL;
	}

	CAutoLock lock(&m_csSocketBufferList);
	if (m_pSocketBufferList)
	{
		pBuffer = m_pSocketBufferList;
		m_pSocketBufferList = m_pSocketBufferList->m_pNext;
		pBuffer->m_pNext = NULL;
		m_nSocketBufferListCount--;
	}
	else
	{
		pBuffer = (CSocketBuffer*)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CSocketBuffer) + DATA_BUF_SIZE);
	}

	if (pBuffer)
	{
		/*I/O����������*/
		pBuffer->m_pBuffer = (PBYTE)(pBuffer + 1);
		pBuffer->m_nBufferLen = nBufferLen;
	}

	return pBuffer;
}

void CIocpServer::ReleaseSocketBuffer(CSocketBuffer* pBuffer)
{
	/*�ͷ�I/O�����ṹ�����ӵ�����I/O�����ṹ�б��������ظ�����*/
	CAutoLock lock(&m_csSocketBufferList);
	if (m_nSocketBufferListCount <= m_nMaxSocketBufferListCount)
	{
		memset(pBuffer, 0, sizeof(CSocketBuffer) + DATA_BUF_SIZE);
		pBuffer->m_pNext = m_pSocketBufferList;
		m_pSocketBufferList = pBuffer;
		m_nSocketBufferListCount++;
	}
	else
	{
		::HeapFree(::GetProcessHeap(), 0, pBuffer);
	}
}

void CIocpServer::FreeSocketBuffer()
{
	/*�ͷſ���I/O�����ṹ�б�*/
	CAutoLock lock(&m_csSocketBufferList);
	CSocketBuffer* pTemp = m_pSocketBufferList;
	CSocketBuffer* pNext;
	while (pTemp)
	{
		pNext = pTemp->m_pNext;
		::HeapFree(::GetProcessHeap(), 0, pTemp);
		pTemp = pNext;
	}
	m_pSocketBufferList = NULL;
	m_nSocketBufferListCount = 0;
}

CSocketContext* CIocpServer::AllocateSocketContext(SOCKET socket)
{
	/*�����׽��������ģ����ȴӿ����׽����������б���ȡ��û���������ڴ�*/
	CSocketContext* pContext = NULL;
	CAutoLock lock(&m_csSocketContextList);
	if (m_pSocketContextList)
	{
		pContext = m_pSocketContextList;
		m_pSocketContextList = m_pSocketContextList->m_pNext;
		pContext->m_pNext = NULL;
		m_nSocketContextListCount--;
	}
	else
	{
		pContext = (CSocketContext*)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CSocketContext));
		::InitializeCriticalSection(&(pContext->m_csLock));
	}
	/*������õ��׽�������������socket�׽���*/
	if (pContext)
	{
		pContext->m_hSocket = socket;
	}

	return pContext;
}

void CIocpServer::ReleaseSocketContext(CSocketContext* pContext)
{
	/*�رո��׽��ֵ�socket*/
	SAFE_RELEASE_SOCKET(pContext->m_hSocket);
	/*�ͷŸ��׽����ϴ�������I/O����*/
	CSocketBuffer* pTemp = NULL;
	while (pContext->m_pWaitingRecv)
	{
		pTemp = pContext->m_pWaitingRecv->m_pNext;
		ReleaseSocketBuffer(pContext->m_pWaitingRecv);
		pContext->m_pWaitingRecv = pTemp;
	}
	pContext->m_nPendingRecv = 0;
	while (pContext->m_pWaitingSend)
	{
		pTemp = pContext->m_pWaitingSend->m_pNext;
		ReleaseSocketBuffer(pContext->m_pWaitingSend);
		pContext->m_pWaitingSend = pTemp;
	}
	pContext->m_nPendingSend = 0;
	/*���ͷŵ����׽������ӵ������б���������*/
	CAutoLock lock(&m_csSocketContextList);
	if (m_nSocketContextListCount <= m_nMaxSocketContextListCount)
	{
		CRITICAL_SECTION csLock = pContext->m_csLock;
		memset(pContext, 0, sizeof(CSocketContext));
		pContext->m_csLock = csLock;
		pContext->m_pNext = m_pSocketContextList;
		m_pSocketContextList = pContext;
		m_nSocketContextListCount++;
	}
	else
	{
		::DeleteCriticalSection(&(pContext->m_csLock));
		::HeapFree(::GetProcessHeap(), 0, pContext);
	}
}

void CIocpServer::FreeSocketContext()
{
	/*�ͷſ����׽����������б�*/
	CAutoLock lock(&m_csSocketContextList);
	CSocketContext* pTemp = m_pSocketContextList;
	CSocketContext* pNext;
	while (pTemp)
	{
		pNext = pTemp->m_pNext;
		::DeleteCriticalSection(&(pTemp->m_csLock));
		::HeapFree(::GetProcessHeap(), 0, pTemp);
		pTemp = pNext;
	}

	m_pSocketContextList = NULL;
	m_nSocketContextListCount = 0;
}

CSocketContext* CIocpServer::FindClient(SOCKET hSocket)
{
	/*�����׽���socket���Ҷ�Ӧ�����׽��ֵ���������Ϣ*/
	CAutoLock lock(&m_csConnectionContext);
	CSocketContext* pContext = m_pConnectionContext;
	while (pContext)
	{
		if (hSocket == pContext->m_hSocket)
		{
			return pContext;
		}
		pContext = pContext->m_pNext;
	}
	return NULL;
}

CSocketBuffer* CIocpServer::GetRecvSocketBuffer(CSocketContext* pContext, CSocketBuffer* pBuffer)
{
	/*��˳���õ�ǰӦ�ö�ȡ��I/O����������������һ���Ѿ����������ӵ��׽��֣����׽�����ӵ�е�����I/O�����ṹӦ�����Ⱥ�˳��*/
	if (pBuffer)
	{
		/*���׽�����һ������I/O���������к��뵱ǰI/O���������к���ͬ��ֱ�Ӷ�ȡ��I/O���������ݻ�����*/
		::EnterCriticalSection(&(pContext->m_csLock));
		if (pBuffer->m_nSerialNo == pContext->m_nCurrentReadSerialNo)
		{
			::LeaveCriticalSection(&(pContext->m_csLock));
			return pBuffer;
		}
		/*������������ˣ�����ǰI/O������˳���ź���*/
		pBuffer->m_pNext = NULL;
		CSocketBuffer* pHead = pContext->m_pWaitingRecv;
		CSocketBuffer* pTemp = NULL;
		while (pHead)
		{
			if (pBuffer->m_nSerialNo > pHead->m_nSerialNo)
			{
				break;
			}
			pTemp = pHead;
			pHead = pHead->m_pNext;
		}
		if (!pTemp)
		{
			/*pBufferӦ�ò��ڱ�ͷ*/
			pBuffer->m_pNext = pContext->m_pWaitingRecv;
			pContext->m_pWaitingRecv = pBuffer;
		}
		else
		{
			/*pBufferӦ�ò��ڱ���*/
			pBuffer->m_pNext = pTemp->m_pNext;
			pTemp->m_pNext = pBuffer;
		}
		pContext->m_nPendingRecv++;
		::LeaveCriticalSection(&(pContext->m_csLock));
	}
	/*ȡ�õ�ǰ��˳��Ӧ��ȡ����I/O���������ݻ�����*/
	CSocketBuffer* pRet = pContext->m_pWaitingRecv;
	if (pRet && (pRet->m_nSerialNo == pContext->m_nCurrentReadSerialNo))
	{
		::EnterCriticalSection(&(pContext->m_csLock));
		pContext->m_pWaitingRecv = pRet->m_pNext;
		pContext->m_nPendingRecv--;
		::LeaveCriticalSection(&(pContext->m_csLock));
		return pRet;
	}
	return NULL;
}

CSocketBuffer* CIocpServer::GetSendSocketBuffer(CSocketContext* pContext, CSocketBuffer* pBuffer)
{
	CSocketBuffer* pRet = pContext->m_pWaitingSend;
	if (pRet)
	{
		::EnterCriticalSection(&(pContext->m_csLock));
		if (pBuffer)
		{
			CSocketBuffer* pTemp = pContext->m_pWaitingSend;
			while (pTemp->m_pNext)
			{
				pTemp = pTemp->m_pNext;
			}
			pTemp->m_pNext = pBuffer;
		}
		pContext->m_pWaitingSend = pRet->m_pNext;
		::LeaveCriticalSection(&(pContext->m_csLock));
		return pRet;
	}
	else
	{
		return pBuffer;
	}
}

unsigned __stdcall CIocpServer::AcceptThreadFunc(LPVOID lpParam)
{
	CIocpServer* pIocpServer = (CIocpServer*)lpParam;
	CSocketBuffer* pBuffer = NULL;
	DWORD dwError = 0;
	// �ʼ��Ͷ�ݳ�ʼ������Accept����
	for (int i = 0; i < pIocpServer->m_pListenContext->m_nInitAccpets; i++)
	{
		pBuffer = pIocpServer->AllocateSocketBuffer(DATA_BUF_SIZE);
		if (!pBuffer)
		{
			myLogConsoleI("%s �߳�%d�˳�...", __FUNCTION__, GetCurrentThreadId());
			return THREAD_EXIT;
		}
		pIocpServer->InsertPendingAccepts(NULL, pBuffer);
		pIocpServer->PostAccept(NULL, pBuffer, dwError);
	}

	int nEventCount = 0;
	pIocpServer->m_hWaitHandle[nEventCount++] = pIocpServer->m_pListenContext->m_hAcceptHandle;
	pIocpServer->m_hWaitHandle[nEventCount++] = pIocpServer->m_pListenContext->m_hRepostHandle;

	while (TRUE)
	{
		/*������رգ��˳��߳�*/
		if (pIocpServer->m_bShutdown)
		{
			myLogConsoleI("%s Accept�߳�%d�˳�...", __FUNCTION__, GetCurrentThreadId());
			return THREAD_EXIT;
		}
		/*��ʱ��m_hWaitHandle���м�������Ԥ�����Ѿ�Ͷ��AcceptEx�����������֮ǰ��ʱ����*/
		DWORD dwWaitRet = ::WSAWaitForMultipleEvents(nEventCount, pIocpServer->m_hWaitHandle, FALSE, 1000, FALSE);
		/*��������ʧ��*/
		if (WSA_WAIT_FAILED == dwWaitRet)
		{
			myLogConsoleI("%s Accept�߳�%d�˳�...", __FUNCTION__, GetCurrentThreadId());
			pIocpServer->Shutdown();
			return THREAD_EXIT;
		}
		/*��ʱ��鵱ǰ���������AcceptEx I/O����ʱ��*/
		else if (WSA_WAIT_TIMEOUT == dwWaitRet)
		{
			pBuffer = pIocpServer->m_pListenContext->m_pAcceptPendingList;
			while (pBuffer)
			{
				int nTimes = 0;
				int nTimesLen = sizeof(int);
				/*��ȡsocket���ӽ���ʱ����ʱ�䳤�Ļ�ֱ�ӶϿ�*/
				::getsockopt(pBuffer->m_hSocket, SOL_SOCKET, SO_CONNECT_TIME, (char*)&nTimes, &nTimesLen);
				if (-1 != nTimes && nTimes > 2)
				{
					SAFE_RELEASE_SOCKET(pBuffer->m_hSocket);
				}
				pBuffer = pBuffer->m_pNext;
			}
		}
		/*��ѯAccept�¼�����*/
		else
		{
			/*dwWaitRet����ֵ����λ��[WSA_WAIT_EVENT_0�� (WSA_WAIT_EVENT_0+ nEventCount - 1)]����Ӧ����������������������*/
			dwWaitRet = dwWaitRet - WSA_WAIT_EVENT_0;
			int nAddAcceptCounts = 0;
			/*��Ӧm_hAcceptHandle�¼�*/
			if (0 == dwWaitRet)
			{
				WSANETWORKEVENTS wsaNetEvent;
				::WSAEnumNetworkEvents(pIocpServer->m_pListenContext->m_hSocket, pIocpServer->m_hWaitHandle[dwWaitRet], &wsaNetEvent);
				/*����FD_ACCEPT�����¼�����ζ��Ͷ�ݵ�Accept��������Ҫ����*/
				if (FD_ACCEPT & wsaNetEvent.lNetworkEvents)
				{
					nAddAcceptCounts = ADD_ACCEPT_COUNT;
				}
			}
			else if (1 == dwWaitRet)
			{
				nAddAcceptCounts = ::InterlockedExchange(&pIocpServer->m_pListenContext->m_nRepostCount, 0);
			}
			/*�����¼������������������󣬹رշ���*/
			else if (dwWaitRet >= nEventCount)
			{
				pIocpServer->Shutdown();
				myLogConsoleI("%s Accept�߳�%d�˳�...", __FUNCTION__, GetCurrentThreadId());
				return THREAD_EXIT;
			}
			/*����AcceptͶ��*/
			int nCount = 0;
			while ((nCount++ < nAddAcceptCounts) && (pIocpServer->m_pListenContext->m_nAcceptPendingListCount < pIocpServer->m_nMaxAccepts))
			{
				pBuffer = pIocpServer->AllocateSocketBuffer(DATA_BUF_SIZE);
				if (pBuffer)
				{
					pIocpServer->InsertPendingAccepts(NULL, pBuffer);
					pIocpServer->PostAccept(NULL, pBuffer, dwError);
				}
			}
		}
	}
	myLogConsoleI("%s Accept�߳�%d�˳�...", __FUNCTION__, GetCurrentThreadId());
	return THREAD_EXIT;
}

unsigned __stdcall CIocpServer::WorkerThreadFunc(LPVOID lpParam)
{
	CIocpServer* pIocpServer = (CIocpServer*)lpParam;
	CSocketBuffer* pBuffer = NULL;
	DWORD dwKey = 0;
	DWORD dwTrans = 0;
	LPOVERLAPPED lpOverlapped = NULL;
	while (TRUE)
	{
		/*��ȡ��ɶ˿ڶ�������*/
		BOOL bRet = ::GetQueuedCompletionStatus(pIocpServer->m_hCompletionPort, &dwTrans, (PULONG_PTR)&dwKey, &lpOverlapped, WSA_INFINITE);
		if (!lpOverlapped)
		{
			myLogConsoleW("%s �����߳�%d�˳�...lpOverlapped", __FUNCTION__, ::GetCurrentThreadId());
			return THREAD_EXIT;
		}
		/*ȡ����ӦlpOverlapped��CSocketBuffer����I/O����*/
		pBuffer = CONTAINING_RECORD(lpOverlapped, CSocketBuffer, m_ol);
		if (!pBuffer)
		{
			continue;
		}
		if (0 == dwKey && enIoAccept != pBuffer->m_ioType)
		{
			myLogConsoleW("%s �����߳�%d�˳�...0 == dwTrans && 0 == dwKey && enIoAccept != pBuffer->m_ioType", __FUNCTION__, GetCurrentThreadId());
			return THREAD_EXIT;
		}
		/*����GetQueuedCompletionStatus���ش���*/
		DWORD dwError = NO_ERROR;
		DWORD dwFlags = 0;
		if (!bRet)
		{
			SOCKET socket = INVALID_SOCKET;
			if (enIoAccept == pBuffer->m_ioType)
			{
				socket = pIocpServer->m_pListenContext->m_hSocket;
			}
			else
			{
				socket = ((CSocketContext*)dwKey)->m_hSocket;
			}
			/*��ѯ���׽ӿ���һ���ص�����ʧ�ܵ�ԭ��*/
			BOOL bResult = ::WSAGetOverlappedResult(socket, &(pBuffer->m_ol), &dwError, FALSE, &dwFlags);
			if (!bResult)
			{
				dwError = ::WSAGetLastError();
				myLogConsoleW("%s WSAGetOverlappedResult����ʧ��", __FUNCTION__);
				myLogConsoleW("%s GetQueuedCompletionStatus����False��������dwError��%d", __FUNCTION__, dwError);
			}
		}
		/*����I/O����*/
		pIocpServer->HandleIo(dwKey, pBuffer, dwTrans, dwError);
	}
	myLogConsoleI("%s �����߳�%d�˳�...", __FUNCTION__, ::GetCurrentThreadId());
	return THREAD_EXIT;
}

/*CIocpClient*/
CIocpClient::CIocpClient()
{

}

CIocpClient::~CIocpClient()
{

}

BOOL CIocpClient::Create(const char* lpSzIp, UINT nPort, UINT nMaxConnections, UINT nThreads, UINT nConcurrency)
{
	InitializeMembers();
	m_nMaxConnections = nMaxConnections;
	if (!InitializeIo())
	{
		myLogConsoleW("%s InitializeIoʧ��", __FUNCTION__);
		return FALSE;
	}
	if (!BeginConnect(lpSzIp, nPort))
	{
		myLogConsoleW("%s BeginConnectʧ��", __FUNCTION__);
		return FALSE;
	}
	if (!BeginThreadPool(nThreads, nConcurrency))
	{
		myLogConsoleW("%s BeginThreadPoolʧ��", __FUNCTION__);
		return FALSE;
	}
	return TRUE;
}

BOOL CIocpClient::BeginConnect(const char* lpSzIp, UINT nPort)
{
	/*�����׽��ֿ�*/
	WSADATA wsaData;
	::WSAStartup(MAKEWORD(2, 2), &wsaData);

	/*��ʼ��socket*/
	SOCKET hSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	myLogConsoleI("%s �׽���%d��ʼ��", __FUNCTION__, hSocket);

	/*���Զ��������Ϣ*/
	SOCKADDR_IN remoteAddr;
	memset(&remoteAddr, 0, sizeof(SOCKADDR_IN));
	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_addr.S_un.S_addr = inet_addr((char*)lpSzIp);
	remoteAddr.sin_port = htons(nPort);

	/*���ӷ�����*/
#if 1
	int nRet = ::connect(hSocket, (struct sockaddr*)(&remoteAddr), sizeof(remoteAddr));
	if (SOCKET_ERROR == nRet)
	{
		SAFE_RELEASE_SOCKET(hSocket);
		myLogConsoleW("%s ���ӷ�����ʧ��", __FUNCTION__);
		return FALSE;
	}
#else
	DWORD dwBytes = 0;
	DWORD dwSends = 0;
	LPFN_CONNECTEX lpConnectEx = nullptr;
	GUID guidConnectEx = WSAID_CONNECTEX;
	::WSAIoctl(hSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidConnectEx,
		sizeof(guidConnectEx),
		&lpConnectEx,
		sizeof(lpConnectEx),
		&dwBytes,
		NULL,
		NULL
	);
	if (lpConnectEx)
	{
		SOCKADDR_IN localAddr;
		localAddr.sin_family = AF_INET;
		localAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
		localAddr.sin_port = htons(0);
		::bind(hSocket, (LPSOCKADDR)&localAddr, sizeof(localAddr));
	}
	OVERLAPPED ol;
	memset(&ol, 0, sizeof(ol));
	BOOL bRet = lpConnectEx(hSocket, (const sockaddr*)&remoteAddr, sizeof(remoteAddr), NULL, 0, &dwSends, &ol);
	if (!bRet)
	{
		DWORD dwError = WSAGetLastError();
		if (ERROR_IO_PENDING != dwError)
		{
			SAFE_RELEASE_SOCKET(hSocket);
			myLogConsoleW("%s ���ӷ�����ʧ��", __FUNCTION__);
			return FALSE;
		}
	}
#endif

	/*���浱ǰ����������*/
	CSocketContext* pContext = AllocateSocketContext(hSocket);
	if (pContext)
	{
		InsertConnectionContext(pContext);
	}
	else
	{
		myLogConsoleW("%s �������Ӵ���", __FUNCTION__);
		return FALSE;
	}

	/*��ɶ˿���socket��*/
	::CreateIoCompletionPort((HANDLE)hSocket, m_hCompletionPort, (DWORD)pContext, 0);

	CSocketBuffer* pNewBuffer = AllocateSocketBuffer(DATA_BUF_SIZE);
	if (pNewBuffer)
	{
		DWORD dwError = 0;
		if (!PostRecv(pContext, pNewBuffer, dwError))
		{
			/*Ͷ��ʧ�ܣ��ر�������*/
			CloseConnectionContext(pContext);
			myLogConsoleW("%s �������׽���%dͶ��Read����ʧ�ܣ��Ͽ�����", __FUNCTION__, pContext->m_hSocket);
		}
	}

	return TRUE;
}

BOOL CIocpClient::Destroy()
{
	Shutdown();
	return TRUE;
}

BOOL CIocpClient::OnReceiveData(CSocketContext* pContext, CSocketBuffer* pBuffer)
{
	myLogConsoleW("%s", __FUNCTION__);
	return __super::OnReceiveData(pContext, pBuffer);
}

BOOL CIocpClient::SendData(SOCKET hSocket, LPCONTEXT_HEAD lpContextHead, LPREQUEST lpRequest, UINT uiMsgType)
{
	CBuffer myDataPool;
	ComposePacket(myDataPool, uiMsgType, lpRequest->m_pDataPtr, sizeof(REQUEST));

	CSocketBuffer* pBuffer = AllocateSocketBuffer(myDataPool.GetBufferLen());
	if (pBuffer)
	{
		memcpy(pBuffer->m_pBuffer, myDataPool.GetBuffer(), myDataPool.GetBufferLen());
		DWORD dwError = 0;
		CSocketContext* pContext = FindClient(hSocket);
		if (pContext)
		{
			return PostSend(pContext, pBuffer, dwError);
		}
		else
		{
			myLogConsoleW("%s û���ҵ���Ӧ����%d���׽�����������Ϣ", __FUNCTION__, hSocket);
			return FALSE;
		}
	}

	return FALSE;
}

BOOL CIocpClient::SendCast(SOCKET hSocket, LPCONTEXT_HEAD lpContextHead, LPREQUEST lpRequest, UINT uiMsgType)
{
	CBuffer myDataPool;
	ComposePacket(myDataPool, uiMsgType, lpRequest->m_pDataPtr, sizeof(REQUEST));

	{
		//CAutoLock lock(&m_csConnectionContext);
		CSocketContext* pContext = m_pConnectionContext;
		while (pContext)
		{
			DWORD dwError = 0;
			CSocketBuffer* pBuffer = AllocateSocketBuffer(myDataPool.GetBufferLen());
			memcpy(pBuffer->m_pBuffer, myDataPool.GetBuffer(), myDataPool.GetBufferLen());
			if (pBuffer)
			{
				PostSend(pContext, pBuffer, dwError);
			}
			pContext = pContext->m_pNext;
		}
	}

	return TRUE;
}