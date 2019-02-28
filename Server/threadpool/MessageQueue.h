#pragma once
#include "amqp.h"
#include "amqp_framing.h"
#include "amqp_tcp_socket.h"

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

/*针对rabbitmq-c的封装*/
typedef int(__stdcall* pRecvMsgCallback)(const std::string& strQueueName, const std::string& strMsg, void* lpParam);
/*RabbitMQ消息包体*/
class CRabbitMsg{
public:
	CRabbitMsg()
	{
		m_pRecvCallback = NULL;
		m_pParam = NULL;
	}

public:
	std::string m_strRouteKey;
	std::string m_strQueueName;
	pRecvMsgCallback m_pRecvCallback;
	void* m_pParam;
};
/*RabbitMQ封装类*/
class CRabbitMQ{
public:
	CRabbitMQ(std::string strHostName, int nPort, std::string strUserName, std::string strPassword, int nTimeOutMs);
	~CRabbitMQ();

public:
	bool ConnectRabbitServer();
	bool Disconnect();
	
	/**
	* @brief         ExchangeDeclare 声明交换机
	* @param[string] strExchange     交换机名称
	* @param[string] strType         交换机类型
	*                "direct"        直接绑定。使用的时候需要将队列绑定到交换机上，消息需要跟指定路由键完全匹配。
	*                "fanout"        广播。使用的时候需要将队列绑定到交换机，消息会转发到所有与该交换机绑定的队列中。
	*                "topic"         匹配。匹配模式。根据路由键匹配相应符合条件的队列，消息会转发到上述所有符合条件的队列中。"#"匹配0个或多个天降，"*"匹配一个条件。
	* @return        true 声明成功   false 声明失败或错误
	*/
	bool ExchangeDeclare(const string &strExchange, const string &strType);

	/**
	* @brief         QueueDeclare    声明消息队列
	* @param[string] strQueueName    消息队列名称
	* @return        true 声明成功   false 声明失败或错误
	*/
	bool QueueDeclare(const string &strQueueName);

	/**
	* @brief         QueueBind       绑定交换机与队列
	* @param[string] strQueueName    队列名称
	* @param[string] strExchange     交换机名称
	* @param[string] strBindKey      路由规则（不能超过255个字节）
	* @return        true 绑定成功 false 绑定失败或错误
	*/
	bool QueueBind(const string &strQueueName, const string &strExchange, const string &strBindKey);

	/**
	* @brief         QueueUnbind     取消指定交换机与队列的绑定
	* @param[string] strQueueName    队列名称
	* @param[string] strExchange     交换机名称
	* @param[string] strBindKey      路由规则（不能超过255个字节）
	* @return        true 解除绑定成功 false 解除绑定失败或错误
	*/
	bool QueueUnbind(const string &strQueueName, const string &strExchange, const string &strBindKey);

	/**
	* @brief         QueueDelete     删除指定队列
	* @param[string] strQueueName    队列名称
	* @return        true 删除队列成功 false 删除队列失败或错误
	*/
	bool QueueDelete(const string &strQueueName);

	/**
	* @brief         SendMsg         发布消息
	* @param[string] strExchange     交换机名称
	* @param[string] strQueueName    队列名称
	* @param[string] strRouteKey     路由规则（不能超过255个字节）
	* @param[string] strMessage      消息字符串
	* @return        true 发布消息成功 false 发布消息失败或错误
	*                                                          
	* @note          在发布消息的时候，一定要确保发送的目的地（交换机/队列）存在以及指定路由有效，否则应该先声明再进行发布消息操作。
	* @note          如果需要实现消息持久化。确保channel/queue的durable以及发布时带的结构amqp_basic_properties_t中字段delivery_mode为持久化属性。
	*/
	bool SendMsg(const std::string& strExchange, const std::string strQueueName, const std::string& strRouteKey, const std::string& strMessage);
	
	/**
	* @brief         RecvMsg         接收消息
	* @param         strQueueName    队列名称
	* @return        true 接收消息成功 false 接收消息失败或错误
	*                                                          
	* @note          在接收消息的时候确保所需要接收消息的队列存在，否则无法接收到消息。
	*/
	bool RecvMsg(const string &strQueueName, struct timeval* timeout);

	/**
	* @brief         HeartBeatFunc   心跳线程函数
	* @return        0 -1
	*/
	static unsigned int __stdcall HeartBeatFunc(LPVOID pParam);
	/**
	* @brief         HeartBeatThread 心跳函数
	* @return        void
	*/
	void HeartBeatThread();

	/**
	* @brief         AddQueue        消费者添加对指定队列接收消息
	* @param[string] strQueueName    队列名称
	# @return        true 成功 false 失败或错误
	*/
	bool AddQueue(const std::string strQueueName);

	/**
	* @brief         StartConsume    开始消费
	* @return        true 成功 false 失败或错误
	*/
	bool StartConsume();

	/**
	* @brief         ConsumeFunc     消费线程函数
	* @return        0 -1
	*/
	static unsigned int __stdcall ConsumeFunc(LPVOID pParam);

	/**
	* @brief         ConsumeThread   消费函数
	* @return        void
	*/
	void ConsumeThread();

	/**
	* @brief         ErrorMsg        错误处理函数
	* @return        ture AMQP_RESPONSE_NORMAL false AMQP Exceptions
	*/
	bool ErrorMsg(amqp_rpc_reply_t x);

private:
	std::string m_strHostName;
	std::string m_strUserName;
	std::string m_strPassword;

	void* m_pConn;
	void* m_pSock;

	int	m_nPort;
	int m_nChannel;

	CCritSec m_csLock;

	struct timeval* m_pConnTimeOut;

	HANDLE m_hHeartBeat;
	unsigned int m_uiHeartBeatThreadId;
	bool m_bHeartBeatFlag;

	HANDLE m_hConsume;
	unsigned int m_uiConsumeThreadId;
	bool m_bConsumeFlag;

	std::vector<std::string> m_vectQueueMap;
};

class CRabbitMQ_Producer{
public:
	CRabbitMQ_Producer();
	~CRabbitMQ_Producer();

private:
	CRabbitMQ* m_objRabbitMQ;
};

class CRabbitMQ_Consumer{
public:
	CRabbitMQ_Consumer();
	~CRabbitMQ_Consumer();

private:
	CRabbitMQ* m_objRabbitMQ;
};