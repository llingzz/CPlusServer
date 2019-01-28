#include "stdafx.h"

CIOCPModule::CIOCPModule(CBaseServer* pServer)
{
	m_pServer = pServer;
	m_hIocp = NULL;
	m_hShutdownEvent = NULL;
	m_nWorkerThreads = 0;
	m_pWorkerThread = NULL;
}
CIOCPModule::~CIOCPModule()
{

}

BOOL CIOCPModule::Initialize()
{
	FILE_INFOS("%s start initialize the iocpmodule...", __FUNCTION__);
	m_hIocp = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (NULL == m_hIocp)
	{
		FILE_ERROR("%s create iocp failed...", __FUNCTION__);
		return FALSE;
	}

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	m_nWorkerThreads = MAX_WORKER_THREADS_PER_PROCESS * systemInfo.dwNumberOfProcessors;
	m_pWorkerThread = new HANDLE[m_nWorkerThreads];

	unsigned int nThreadId = 0;
	for (auto i = 0; i < m_nWorkerThreads; i++)
	{
		LPWORKER_THREAD pWorkerThread = new WORKER_THREAD;
		pWorkerThread->nThreadId = i;
		pWorkerThread->pCIOCPModule = this;

		m_pWorkerThread[i] = (HANDLE)::_beginthreadex(NULL, 0, WorkerThreadFunc, (void*)pWorkerThread, 0, &nThreadId);
		if (NULL == m_pWorkerThread[i])
		{
			FILE_ERROR("%s create workerthread failed at threadId :%d :%d", __FUNCTION__, nThreadId, pWorkerThread->nThreadId);
			return FALSE;
		}
	}

	FILE_INFOS("%s create %d workerthread successful...", __FUNCTION__, m_nWorkerThreads);
	FILE_INFOS("%s initialize the iocpmodule successful...", __FUNCTION__);

	return TRUE;
}

unsigned int __stdcall CIOCPModule::WorkerThreadFunc(LPVOID lpParam)
{
	LPWORKER_THREAD pWorkerThread = (LPWORKER_THREAD)lpParam;
	FILE_INFOS("%s the workerthread with id:%d started...", __FUNCTION__, pWorkerThread->nThreadId);
	//printf("%s the workerthread with id:%d started...\n", __FUNCTION__, pWorkerThread->nThreadId);

	OVERLAPPED* pOverlapped = NULL;
	LPSOCKET_CONTEXT pSocketContext = NULL;
	DWORD dwByteTransfered = 0;

	while (WAIT_OBJECT_0 != WaitForSingleObject(pWorkerThread->pCIOCPModule->m_hShutdownEvent, 0))
	{
		auto bRet = ::GetQueuedCompletionStatus(
			pWorkerThread->pCIOCPModule->m_hIocp,
			&dwByteTransfered,
			(PULONG_PTR)&pSocketContext,
			&pOverlapped,
			INFINITE);

		if (NULL == pSocketContext)
		{
			break;
		}

		if (!bRet)
		{
			DWORD dwErr = GetLastError();
			if (!pWorkerThread->pCIOCPModule->HandleErrors(pSocketContext, dwErr))
			{
				break;
			}

			continue;
		}
		else
		{
			LPIO_CONTEXT pIoContext = CONTAINING_RECORD(pOverlapped, IO_CONTEXT, m_Overlapped);
			//判断客户端是否断开
			if (0 == dwByteTransfered && (OPE_RECV == pIoContext->m_OpeType || OPE_SEND == pIoContext->m_OpeType))
			{
				FILE_INFOS("client with ip:%s: port:%d disconnected...", inet_ntoa(pSocketContext->m_SockAddrIn.sin_addr), ntohs(pSocketContext->m_SockAddrIn.sin_port));
				pWorkerThread->pCIOCPModule->m_pServer->CloseClients(pSocketContext->m_Socket);
				pWorkerThread->pCIOCPModule->RemoveSocketContext(pSocketContext);
				continue;
			}
			else
			{
				switch ((OPE_TYPE)pIoContext->m_OpeType)
				{
				case OPE_ACCEPT:
					pWorkerThread->pCIOCPModule->m_pServer->m_IocpAccept->OnMessageHandle(pIoContext->m_OpeType, pSocketContext, pIoContext, pWorkerThread->pCIOCPModule->m_pServer);
					break;
				case OPE_RECV:
				case OPE_SEND:
					pWorkerThread->pCIOCPModule->m_pServer->m_IocpSocket->OnMessageHandle(pIoContext->m_OpeType, pSocketContext, pIoContext, pWorkerThread->pCIOCPModule->m_pServer);
					break;
				default:
					FILE_WARNS("%s pIoContext->m_OpType of workerthread params encounter a problem...", __FUNCTION__);
					break;
				}
			}
		}
	}

	FILE_INFOS("%s workerthread with id:%d exit...", __FUNCTION__, pWorkerThread->nThreadId);
	SAFE_DELETE(lpParam);

	return 0;
}

void CIOCPModule::RemoveSocketContext(LPSOCKET_CONTEXT pSocketContext)
{
	CAutoLock lock(&m_pServer->m_csVectClientContext);
	std::vector<LPSOCKET_CONTEXT>::iterator iter;
	for (iter = m_pServer->m_vectClientConetxt.begin(); iter != m_pServer->m_vectClientConetxt.end();)
	{
		if (*iter == pSocketContext)
		{
			delete pSocketContext;
			pSocketContext = NULL;
			iter = m_pServer->m_vectClientConetxt.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

BOOL CIOCPModule::HandleErrors(LPSOCKET_CONTEXT pSocketContext, const DWORD& dwErr)
{
	//超时
	if (WAIT_TIMEOUT == dwErr)
	{
		if ((::send(pSocketContext->m_Socket, "", 0, 0) == -1))
		{
			FILE_ERROR("%s the client exit with exception...exit thread with:%d...", __FUNCTION__, dwErr);
			FILE_ERROR("%s client with ip:%s port:%d disconnected...", __FUNCTION__, inet_ntoa(pSocketContext->m_SockAddrIn.sin_addr), ntohs(pSocketContext->m_SockAddrIn.sin_port));
			this->m_pServer->CloseClients(pSocketContext->m_Socket);
			this->RemoveSocketContext(pSocketContext);
			return TRUE;
		}
		else
		{
			FILE_ERROR("%s the net request is timeout, retrying...", __FUNCTION__);
			return TRUE;
		}
	}

	else if (ERROR_NETNAME_DELETED == dwErr)
	{
		FILE_ERROR("%s the client exit with exception...exit thread with:%d...", __FUNCTION__, dwErr);
		FILE_ERROR("%s client with ip:%s port:%d disconnected...", __FUNCTION__, inet_ntoa(pSocketContext->m_SockAddrIn.sin_addr), ntohs(pSocketContext->m_SockAddrIn.sin_port));
		this->m_pServer->CloseClients(pSocketContext->m_Socket);
		this->RemoveSocketContext(pSocketContext);
		return TRUE;
	}

	else
	{
		FILE_ERROR("%s the client exit with exception...exit thread with:%d...", __FUNCTION__, dwErr);
		FILE_ERROR("%s client with ip:%s port:%d disconnected...", __FUNCTION__, inet_ntoa(pSocketContext->m_SockAddrIn.sin_addr), ntohs(pSocketContext->m_SockAddrIn.sin_port));
		this->m_pServer->CloseClients(pSocketContext->m_Socket);
		return FALSE;
	}
}