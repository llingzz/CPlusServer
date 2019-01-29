#pragma once

typedef struct _tagMQ_MESSAGE{
	int nMessageType;
	int nMessageID;
	char szMessageContent[DATA_BUF_SIZE];
}MQ_MESSAGE, *LPMQ_MESSAGE;

/*发布/订阅实现消息队列*/
class MQ_Subscriber;
typedef std::multimap<int, MQ_Subscriber*> CSubscriberMultiMap;
typedef std::deque<MQ_MESSAGE> CMessageDeque;
typedef std::vector<MQ_MESSAGE> CMessageVector;
class MQ_Manager;
//发布者
class MQ_Publisher{
public:
	MQ_Publisher(){};
	~MQ_Publisher(){};

public:
	void PublishMQ(MQ_Manager* pMQManager, int nMessageID, LPMQ_MESSAGE pMessage);
};
//订阅者
class MQ_Subscriber{
public:
	MQ_Subscriber(){};
	~MQ_Subscriber(){};

public:
	void SubscribMQ(MQ_Manager* pMQManager, int nMessageID);
	void UnSubscribMQ(MQ_Manager* pMQManager, int nMessageID);

	void HandleMessage(LPMQ_MESSAGE pMessage);

private:
	CMessageVector			m_vectorMessage;
};
//消息队列
class MQ_Manager{
public:
	MQ_Manager();
	~MQ_Manager(){};

public:
	void AddSubscriber(MQ_Subscriber* pMQSubscriber, int nMessageID);
	void RemoveSubscriber(MQ_Subscriber* pMQSubscriber, int nMessageID);

	void RecvMessage(int nMessageID, LPMQ_MESSAGE pMessage);
	void SendMessage(MQ_Subscriber* pMQSubscriber, LPMQ_MESSAGE pMessage);

	static unsigned __stdcall HandleThreadFunc(LPVOID lpParam);
	void HandleMessage(MQ_MESSAGE message);

public:
	HANDLE					m_hHandleThread;
	CCritSec				m_csSubscriberMapLock;
	CSubscriberMultiMap		m_mapSubscriber;
	CCritSec				m_csMessageDequeLock;
	CMessageDeque			m_dequeMessage;
};

/*生产/消费实现消息队列，双缓冲消息队列*/
typedef BOOL (*LPCALLBACK_CONSUME_FUNC)(LPMQ_MESSAGE pMessage);
class CMessageQueue{
public:
	CMessageQueue();
	~CMessageQueue();

public:
	void Produce(LPMQ_MESSAGE pMessage);
	void Consume(LPMQ_MESSAGE pMessage);

	static unsigned __stdcall ProduceMessageThread(LPVOID pParam);
	static unsigned __stdcall ConsumeMessageThread(LPVOID pParam);

	CMessageQueue* SetCallbackFunc(LPCALLBACK_CONSUME_FUNC pCallbackFunc);

private:
	CCritSec		m_csProduceLock;
	CMessageDeque	m_dequeProduce;
	unsigned int	m_uiProduceThreadID;
	HANDLE			m_hProduceThread;

	CCritSec		m_csConsumeLock;
	CMessageDeque	m_dequeConsume;
	unsigned int	m_uiConsumeThreadID;
	HANDLE			m_hConsumeThread;
	
	HANDLE			m_hConsumeEvent;
	HANDLE			m_hProduceEvent;

	LPCALLBACK_CONSUME_FUNC m_pCallbackFunc;
};
/*CMessageQueue单例类*/
class CMessageQueueEx : public CMessageQueue, public CSingle<CMessageQueueEx>{
public:
	CMessageQueueEx(){}
	~CMessageQueueEx(){}
};