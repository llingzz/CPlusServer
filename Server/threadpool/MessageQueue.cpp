#include "stdafx.h"

void MQ_Publisher::PublishMQ(MQ_Manager* pMQManager, int nMessageID, LPMQ_MESSAGE pMessage)
{
	pMQManager->RecvMessage(nMessageID, pMessage);
}

void MQ_Subscriber::SubscribMQ(MQ_Manager* pMQManager, int nMessageID)
{
	pMQManager->AddSubscriber(this, nMessageID);
}
void MQ_Subscriber::UnSubscribMQ(MQ_Manager* pMQManager, int nMessageID)
{
	pMQManager->RemoveSubscriber(this, nMessageID);
}
void MQ_Subscriber::HandleMessage(LPMQ_MESSAGE pMessage)
{
	printf("Deal Message with nMessageID:%d, nMessageType:%d, szMessageContent:%s.\n", pMessage->nMessageID, pMessage->nMessageType, pMessage->szMessageContent);
}

MQ_Manager::MQ_Manager()
{
	unsigned int uiThreadID = 0;
	m_hHandleThread = (HANDLE)::_beginthreadex(NULL, 0, HandleThreadFunc, this, 0, &uiThreadID);
}
void MQ_Manager::AddSubscriber(MQ_Subscriber* pMQSubscriber, int nMessageID)
{
	CAutoLock lock(&m_csSubscriberMapLock);
	m_mapSubscriber.insert(std::make_pair(nMessageID, pMQSubscriber));
}
void MQ_Manager::RemoveSubscriber(MQ_Subscriber* pMQSubscriber, int nMessageID)
{
	CAutoLock lock(&m_csSubscriberMapLock);
	if (m_mapSubscriber.empty())
	{
		return;
	}
	std::multimap<int, MQ_Subscriber*>::iterator iter = m_mapSubscriber.lower_bound(nMessageID);
	while (iter != m_mapSubscriber.upper_bound(nMessageID))
	{
		if (iter->second == pMQSubscriber)
		{
			m_mapSubscriber.erase(iter);
			break;
		}
		iter++;
	}
	return;
}
void MQ_Manager::RecvMessage(int nMessageID, LPMQ_MESSAGE pMessage)
{
	CAutoLock lock(&m_csMessageDequeLock);
	m_dequeMessage.push_back(*((LPMQ_MESSAGE)pMessage));
}
void MQ_Manager::SendMessage(MQ_Subscriber* pMQSubscriber, LPMQ_MESSAGE pMessage)
{
	pMQSubscriber->HandleMessage(pMessage);
}
unsigned MQ_Manager::HandleThreadFunc(LPVOID lpParam)
{
	MQ_Manager* pManager = (MQ_Manager*)lpParam;

	while (TRUE)
	{
		while (!pManager->m_dequeMessage.empty())
		{
			CAutoLock lock(&(pManager->m_csMessageDequeLock));
			MQ_MESSAGE message = pManager->m_dequeMessage.front();
			pManager->m_dequeMessage.pop_front();
			pManager->HandleMessage(message);
		}
	}
}
void MQ_Manager::HandleMessage(MQ_MESSAGE message)
{
	CAutoLock lock(&m_csSubscriberMapLock);
	std::multimap<int, MQ_Subscriber*>::iterator iter = m_mapSubscriber.lower_bound(message.nMessageID);
	for (; iter != m_mapSubscriber.upper_bound(message.nMessageID);iter++)
	{
		SendMessage(iter->second, &message);
	}
}