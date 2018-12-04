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
	//LOG_INFO("CIOCPModule::Initialize Start Initialize the Iocp Module...");
	m_hIocp = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (NULL == m_hIocp)
	{
		//LOG_ERROR("Create Iocp Failed...");
		return FALSE;
	}

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	m_nWorkerThreads = MAX_WORKER_THREADS_PER_PROCESS * systemInfo.dwNumberOfProcessors;
	m_pWorkerThread = new HANDLE[m_nWorkerThreads];

	unsigned long nThreadId = 0;
	for (auto i = 0; i < m_nWorkerThreads; i++)
	{
		LPWORKER_THREAD pWorkerThread = new WORKER_THREAD;
		pWorkerThread->nThreadId = i;
		pWorkerThread->pCIOCPModule = this;

		m_pWorkerThread[i] = ::CreateThread(NULL, 0, WorkerThreadFunc, (void*)pWorkerThread, 0, &nThreadId);
		if (NULL == m_pWorkerThread[i])
		{
			//LOG_ERROR("Create WorkerThread Failed at ThreadId : " << nThreadId << " : " << pWorkerThread->nThreadId);
			return FALSE;
		}
	}

	//LOG_INFO("Create " << m_nWorkerThreads << " WorkerThread Successful...");
	//LOG_INFO("CIOCPModule::Initialize Initialize the Iocp Module Successful...");

	return TRUE;
}

DWORD __stdcall CIOCPModule::WorkerThreadFunc(LPVOID lpParam)
{
	LPWORKER_THREAD pWorkerThread = (LPWORKER_THREAD)lpParam;
	//LOG_INFO("The WorkerThread with Id : " << pWorkerThread->nThreadId << " Started...");

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
			//�жϿͻ����Ƿ�Ͽ�
			if (0 == dwByteTransfered && (OPE_RECV == pIoContext->m_OpeType || OPE_SEND == pIoContext->m_OpeType))
			{
				//LOG_ERROR("Client: " << inet_ntoa(pSocketContext->m_SockAddrIn.sin_addr) << " : " << ntohs(pSocketContext->m_SockAddrIn.sin_port) << " Disconnected...");
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
					//LOG_ERROR("pIoContext->m_OpType of Worker Thread's Params encounter A Problem...");
					break;
				}
			}
		}
	}

	//LOG_INFO("Worker Thread with ID:" << pWorkerThread->nThreadId << " Exit...");
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
	//��ʱ
	if (WAIT_TIMEOUT == dwErr)
	{
		if ((::send(pSocketContext->m_Socket, "", 0, 0) == -1))
		{
			//LOG_INFO("The Client exit with Exception...Exit Thread with�� " << dwErr);
			this->RemoveSocketContext(pSocketContext);
			return TRUE;
		}
		else
		{
			//LOG_INFO("The Net Request is Timeout, Retrying...");
			return TRUE;
		}
	}

	else if (ERROR_NETNAME_DELETED == dwErr)
	{
		//LOG_ERROR("The Client exit with Exception...Exit Thread with�� " << dwErr);
		this->RemoveSocketContext(pSocketContext);
		return TRUE;
	}

	else
	{
		//LOG_ERROR("Finish IoComletionPort Failed...Exit Thread with�� " << dwErr);
		return FALSE;
	}
}