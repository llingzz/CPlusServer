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
	myLogConsoleI("%s Deal Message with nMessageID:%d, nMessageType:%d, szMessageContent:%s...", __FUNCTION__, pMessage->nMessageID, pMessage->nMessageType, pMessage->szMessageContent);
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

CMessageQueue::CMessageQueue()
{
	m_hConsumeEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hProduceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hProduceThread = (HANDLE)::_beginthreadex(NULL, 0, ProduceMessageThread, this, 0, &m_uiProduceThreadID);
	m_hConsumeThread = (HANDLE)::_beginthreadex(NULL, 0, ConsumeMessageThread, this, 0, &m_uiConsumeThreadID);
}
CMessageQueue::~CMessageQueue()
{
	::CloseHandle(m_hProduceEvent);
	::CloseHandle(m_hConsumeEvent);
	::CloseHandle(m_hProduceThread);
	::CloseHandle(m_hConsumeThread);
}
void CMessageQueue::Produce(LPMQ_MESSAGE pMessage)
{
	CAutoLock lock(&m_csProduceLock);
	m_dequeProduce.push_back(*pMessage);
}
void CMessageQueue::Consume(LPMQ_MESSAGE pMessage)
{
	m_pCallbackFunc(pMessage);
}
unsigned __stdcall CMessageQueue::ProduceMessageThread(LPVOID pParam)
{
	CMessageQueue* pMessageQueue = (CMessageQueue*)pParam;
	while (TRUE)
	{
		if (WAIT_OBJECT_0 == ::WaitForSingleObject(pMessageQueue->m_hConsumeEvent, 0))
		{
			CAutoLock lock(&pMessageQueue->m_csProduceLock);
			if (pMessageQueue->m_dequeProduce.empty())
			{
				continue;
			}
			std::swap(pMessageQueue->m_dequeProduce, pMessageQueue->m_dequeConsume);
			::SetEvent(pMessageQueue->m_hProduceEvent);
		}
	}

	return 0;
}
unsigned __stdcall CMessageQueue::ConsumeMessageThread(LPVOID pParam)
{
	CMessageQueue* pMessageQueue = (CMessageQueue*)pParam;
	while (TRUE)
	{
		{
			CAutoLock lock(&pMessageQueue->m_csConsumeLock);
			if (pMessageQueue->m_dequeConsume.empty())
			{
				::SetEvent(pMessageQueue->m_hConsumeEvent);
				::WaitForSingleObject(pMessageQueue->m_hProduceEvent, INFINITE);
			}
			else
			{
				MQ_MESSAGE stuMessage = pMessageQueue->m_dequeConsume.front();
				pMessageQueue->Consume(&stuMessage);
				pMessageQueue->m_dequeConsume.pop_front();
			}
		}
	}

	return 0;
}
CMessageQueue* CMessageQueue::SetCallbackFunc(LPCALLBACK_CONSUME_FUNC pCallbackFunc)
{
	m_pCallbackFunc = pCallbackFunc;
	return this;
}

/*针对rabbitmq-c的封装*/
CRabbitMQ::CRabbitMQ(std::string strHostName, int nPort, std::string strUserName, std::string strPassword, int nTimeOutMs)
{
	m_strHostName = strHostName;
	m_strUserName = strUserName;
	m_strPassword = strPassword;

	m_nPort = nPort;

	m_nChannel = 1;

	m_pConnTimeOut = NULL;
	if (nTimeOutMs > 0)
	{
		m_pConnTimeOut = new struct timeval();
		m_pConnTimeOut->tv_sec = nTimeOutMs / 1000;
		m_pConnTimeOut->tv_usec = (nTimeOutMs % 1000) * 1000;
	}

	m_pConn = NULL;
	m_pSock = NULL;

	m_hHeartBeat = NULL;
	m_uiHeartBeatThreadId = 0;
	m_bHeartBeatFlag = false;

	m_hConsume = NULL;
	m_uiConsumeThreadId = 0;
	m_bConsumeFlag = false;
}
CRabbitMQ::~CRabbitMQ()
{
	m_pConn = NULL;
	delete m_pConn;
	m_pSock = NULL;
	delete m_pSock;
}
bool CRabbitMQ::ConnectRabbitServer()
{
	CAutoLock lock(&m_csLock);

	if (m_pConn)
	{
		myLogConsoleI("%s m_pConn is NULL...", __FUNCTION__);
		myLogFileI("%s m_pConn is NULL...", __FUNCTION__);
		Disconnect();
		return false;
	}
	m_pConn = amqp_new_connection();
	m_pSock = amqp_tcp_socket_new((amqp_connection_state_t)m_pConn);
	if (NULL == m_pSock)
	{
		myLogConsoleI("%s amqp_tcp_socket_new failed......", __FUNCTION__);
		myLogFileI("%s amqp_tcp_socket_new failed......", __FUNCTION__);
		Disconnect();
		return false;
	}

	int nStatus = amqp_socket_open_noblock((amqp_socket_t*)m_pSock, m_strHostName.c_str(), m_nPort, m_pConnTimeOut);
	if (nStatus < 0)
	{
		myLogConsoleI("%s amqp_socket_open_noblock failed... %s", __FUNCTION__);
		myLogFileI("%s amqp_socket_open_noblock failed......", __FUNCTION__);
		Disconnect();
		return false;
	}

	amqp_rpc_reply_t reply = amqp_login((amqp_connection_state_t)m_pConn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, m_strUserName.c_str(), m_strPassword.c_str());
	if (AMQP_RESPONSE_NORMAL != reply.reply_type)
	{
		myLogConsoleI("%s amqp_login failed...", __FUNCTION__);
		myLogFileI("%s amqp_login failed......", __FUNCTION__);
		amqp_connection_close_t* test = (amqp_connection_close_t*)reply.reply.decoded;
		Disconnect();
		return false;
	}

	if (NULL == m_hHeartBeat)
	{
		m_bHeartBeatFlag = true;
		m_hHeartBeat = (HANDLE)_beginthreadex(NULL, 0, HeartBeatFunc, this, 0, &m_uiHeartBeatThreadId);
		Sleep(200);
	}

	return true;
}
bool CRabbitMQ::Disconnect()
{
	CAutoLock lock(&m_csLock);

	amqp_rpc_reply_t reply = amqp_channel_close((amqp_connection_state_t)m_pConn, m_nChannel, AMQP_REPLY_SUCCESS);
	if (AMQP_RESPONSE_NORMAL != reply.reply_type)
	{
		myLogConsoleI("%s amqp_channel_close failed...", __FUNCTION__);
		myLogFileI("%s amqp_channel_close failed......", __FUNCTION__);

		return false;
	}
	reply = amqp_connection_close((amqp_connection_state_t)m_pConn, AMQP_REPLY_SUCCESS);
	if (AMQP_RESPONSE_NORMAL != reply.reply_type)
	{
		myLogConsoleI("%s amqp_connection_close failed...", __FUNCTION__);
		myLogFileI("%s amqp_connection_close failed......", __FUNCTION__);

		return false;
	}

	int nStatus = amqp_destroy_connection((amqp_connection_state_t)m_pConn);
	if (nStatus < 0)
	{
		myLogConsoleI("%s amqp_destroy_connection failed...", __FUNCTION__);
		myLogFileI("%s amqp_destroy_connection failed......", __FUNCTION__);

		return false;
	}

	m_pConn = NULL;
	delete m_pConn;
	m_pSock = NULL;
	delete m_pSock;

	m_bHeartBeatFlag = false;
	if (NULL != m_hHeartBeat)
	{
		WaitForSingleObject(m_hHeartBeat, 1000);
		CloseHandle(m_hHeartBeat);
		m_hHeartBeat = NULL;
	}

	m_bConsumeFlag = false;
	if (NULL != m_hConsume)
	{
		WaitForSingleObject(m_hConsume, 1000);
		CloseHandle(m_hConsume);
		m_hConsume = NULL;
	}

	return true;
}
bool CRabbitMQ::ExchangeDeclare(const string &strExchange, const string &strType)
{
	amqp_channel_open((amqp_connection_state_t)m_pConn, m_nChannel);	// 开启连接通道
	amqp_bytes_t _exchange = amqp_cstring_bytes(strExchange.c_str());	// 交换机名称
	amqp_bytes_t _type = amqp_cstring_bytes(strType.c_str());			// 交换机类型（不能为""）
	int32_t _passive = 0;												// 1 交换机之前如果不存在返回失败 0 默认
	int32_t _durable = 0;												// 1 持久化 0 相反
	int32_t _auto_delete = 0;											// 1 解除绑定后删除队列 0 相反
	int32_t _iternal = 0;												// 0 可以直接使用 1 只能绑定到其他交换机使用
	amqp_exchange_declare((amqp_connection_state_t)m_pConn, m_nChannel, _exchange, _type, _passive, _durable, _auto_delete, _iternal, amqp_empty_table);
	if (!ErrorMsg(amqp_get_rpc_reply((amqp_connection_state_t)m_pConn)))
	{
		amqp_channel_close((amqp_connection_state_t)m_pConn, m_nChannel, AMQP_REPLY_SUCCESS);
		return false;
	}

	amqp_channel_close((amqp_connection_state_t)m_pConn, m_nChannel, AMQP_REPLY_SUCCESS);	// 关闭连接通道
	return true;
}
bool CRabbitMQ::QueueDeclare(const string &strQueueName)
{
	if (NULL == m_pConn) 
	{
		myLogConsoleI("%s m_pConn is null", __FUNCTION__);
		myLogFileI("%s m_pConn is null", __FUNCTION__);
		return false;
	}

	amqp_channel_open((amqp_connection_state_t)m_pConn, m_nChannel);	// 开启连接通道
	amqp_bytes_t _queue = amqp_cstring_bytes(strQueueName.c_str());		// 队列名称
	int32_t _passive = 0;												// 1 队列之前不存在返回失败 0 默认
	int32_t _durable = 0;												// 1 持久化 0 相反
	int32_t _exclusive = 0;												// 1 当前连接关闭，自动删除队列 0 相反
	int32_t _auto_delete = 0;											// 1 没有消费者时，自动删除队列 0 相反
	amqp_queue_declare((amqp_connection_state_t)m_pConn, m_nChannel, _queue, _passive, _durable, _exclusive, _auto_delete, amqp_empty_table);
	if (!ErrorMsg(amqp_get_rpc_reply((amqp_connection_state_t)m_pConn)))
	{
		amqp_channel_close((amqp_connection_state_t)m_pConn, m_nChannel, AMQP_REPLY_SUCCESS);
		return false;
	}

	amqp_channel_close((amqp_connection_state_t)m_pConn, m_nChannel, AMQP_REPLY_SUCCESS);	//关闭连接通道
	return true;
}
bool CRabbitMQ::QueueBind(const string &strQueueName, const string &strExchange, const string &strBindKey) {
	if (NULL == m_pConn) 
	{
		myLogConsoleI("%s m_pConn is null", __FUNCTION__);
		myLogFileI("%s m_pConn is null", __FUNCTION__);
		return false;
	}

	amqp_channel_open((amqp_connection_state_t)m_pConn, m_nChannel);	// 开启连接通道
	amqp_bytes_t _queue = amqp_cstring_bytes(strQueueName.c_str());		// 队列名称
	amqp_bytes_t _exchange = amqp_cstring_bytes(strExchange.c_str());	// 交换机名称
	amqp_bytes_t _routkey = amqp_cstring_bytes(strBindKey.c_str());		// 路由规则
	amqp_queue_bind((amqp_connection_state_t)m_pConn, m_nChannel, _queue, _exchange, _routkey, amqp_empty_table);
	if (!ErrorMsg(amqp_get_rpc_reply((amqp_connection_state_t)m_pConn)))
	{
		amqp_channel_close((amqp_connection_state_t)m_pConn, m_nChannel, AMQP_REPLY_SUCCESS);
		return false;
	}

	amqp_channel_close((amqp_connection_state_t)m_pConn, m_nChannel, AMQP_REPLY_SUCCESS);	// 关闭连接通道
	return true;
}
bool CRabbitMQ::QueueUnbind(const string &strQueueName, const string &strExchange, const string &strBindKey)
{
	if (NULL == m_pConn) 
	{
		myLogConsoleI("%s m_pConn is null", __FUNCTION__);
		myLogFileI("%s m_pConn is null", __FUNCTION__);
		return false;
	}

	amqp_channel_open((amqp_connection_state_t)m_pConn, m_nChannel);	// 开启连接通道
	amqp_bytes_t _queue = amqp_cstring_bytes(strQueueName.c_str());		// 队列名称
	amqp_bytes_t _exchange = amqp_cstring_bytes(strExchange.c_str());	// 交换机名称
	amqp_bytes_t _routkey = amqp_cstring_bytes(strBindKey.c_str());		// 路由规则
	amqp_queue_unbind((amqp_connection_state_t)m_pConn, m_nChannel, _queue, _exchange, _routkey, amqp_empty_table);
	if (!ErrorMsg(amqp_get_rpc_reply((amqp_connection_state_t)m_pConn)))
	{
		amqp_channel_close((amqp_connection_state_t)m_pConn, m_nChannel, AMQP_REPLY_SUCCESS);
		return false;
	}

	amqp_channel_close((amqp_connection_state_t)m_pConn, m_nChannel, AMQP_REPLY_SUCCESS);	// 关闭连接通道
	return true;
}
bool CRabbitMQ::QueueDelete(const string &strQueueName)
{
	if (NULL == m_pConn) 
	{
		myLogConsoleI("%s m_pConn is null", __FUNCTION__);
		myLogFileI("%s m_pConn is null", __FUNCTION__);
		return false;
	}

	amqp_channel_open((amqp_connection_state_t)m_pConn, m_nChannel);	// 开启连接通道
	amqp_bytes_t _queue = amqp_cstring_bytes(strQueueName.c_str());		// 队列名称
	int32_t _unused = 0;												// _unused
	int32_t _if_empty = 0;												// _if_empty
	amqp_queue_delete((amqp_connection_state_t)m_pConn, m_nChannel, _queue, _unused, _if_empty);
	if (!ErrorMsg(amqp_get_rpc_reply((amqp_connection_state_t)m_pConn)))
	{
		amqp_channel_close((amqp_connection_state_t)m_pConn, m_nChannel, AMQP_REPLY_SUCCESS);
		return false;
	}

	amqp_channel_close((amqp_connection_state_t)m_pConn, m_nChannel, AMQP_REPLY_SUCCESS);	// 关闭连接通道
	return true;
}
bool CRabbitMQ::SendMsg(const std::string& strExchange, const std::string strQueueName, const std::string& strRouteKey, const std::string& strMessage)
{
	if (strMessage.size() <= 0)
	{
		myLogConsoleI("%s strMessage.size() <= 0 ...", __FUNCTION__);
		myLogFileI("%s strMessage.size() <= 0 ......", __FUNCTION__);
		return false;
	}

	amqp_channel_open((amqp_connection_state_t)m_pConn, m_nChannel);				// 开启连接通道
	if (!ErrorMsg(amqp_get_rpc_reply((amqp_connection_state_t)m_pConn)))
	{
		amqp_channel_close((amqp_connection_state_t)m_pConn, m_nChannel, AMQP_REPLY_SUCCESS);
		return false;
	}

	CAutoLock lock(&m_csLock);

	if (NULL == m_pConn)
	{
		if (!ConnectRabbitServer())
		{
			myLogConsoleI("%s ReConnectRabbitServer failed...", __FUNCTION__);
			myLogFileI("%s ReConnectRabbitServer failed......", __FUNCTION__);
			return false;
		}
	}

	amqp_basic_properties_t props;													// 发布消息的属性
	props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;	// _flags
	props.content_type = amqp_cstring_bytes("text/plain");							// 消息为文本消息
	props.delivery_mode = 2;														// 持久化模式

	amqp_bytes_t _exchange = amqp_cstring_bytes(strExchange.c_str());				// 交换机名称
	amqp_bytes_t _route_key = amqp_cstring_bytes(strRouteKey.c_str());				// 路由规则
	amqp_bytes_t _message = amqp_cstring_bytes(strMessage.c_str());					// 需要发布的消息
	int32_t _mandatory = 0;															// 1 如果交换机根据自身属性和消息的路由规则无法找到一个符合条件的队列，消息会返还回来
																					// 0 出现上述情况时直接将消息丢弃
	int32_t _immediate = 0;															// 1 如果与该消息对应的队列上没有消费者或者说所有queue都没有消费者，消息会返还回来
																					// 0 出现上述情况仍然将消息放入消息队列
	int nRet = amqp_basic_publish((amqp_connection_state_t)m_pConn, m_nChannel, _exchange, _route_key, _mandatory, _immediate, &props, _message);
	if (nRet < 0)
	{
		// 发布消息失败，自动补发
		if (ConnectRabbitServer())
		{
			myLogConsoleI("%s retrying to publish message...", __FUNCTION__);
			myLogFileI("%s retrying to publish message......", __FUNCTION__);
			auto nTimes = 1;
			while (amqp_basic_publish((amqp_connection_state_t)m_pConn, m_nChannel, _exchange, _route_key, _mandatory, _immediate, &props, _message) < 0)
			{
				myLogConsoleI("%s retrying to publish message...retry times %d ", __FUNCTION__, nTimes);
				myLogFileI("%s retrying to publish message......retry times %d ", __FUNCTION__, nTimes);
				nTimes++;
				Sleep(200);
			}
		}
	}
	amqp_channel_close((amqp_connection_state_t)m_pConn, m_nChannel, AMQP_REPLY_SUCCESS);	// 关闭连接通道

	return true;
}
bool CRabbitMQ::RecvMsg(const string &strQueueName, struct timeval* timeout)
{
	if (NULL == m_pConn)
	{
		myLogConsoleI("%s m_pConn is null", __FUNCTION__);
		myLogFileI("%s m_pConn is null", __FUNCTION__);
		return false;
	}

	amqp_channel_open((amqp_connection_state_t)m_pConn, m_nChannel);										// 开启连接通道
	if (!ErrorMsg(amqp_get_rpc_reply((amqp_connection_state_t)m_pConn)))
	{
		amqp_channel_close((amqp_connection_state_t)m_pConn, m_nChannel, AMQP_REPLY_SUCCESS);
		return false;
	}

	int32_t _prefetch_size = 0;																				// 默认为0
	int16_t _prefetch_count = 1;																			// 表示rabbitmq不会同时给一个消费者推送多于_prefetch_count个消息，即如果有_prefetch_count和消息还没有ack，则该consumer将阻塞，知道有消息ack
	int32_t _global = 0;																					// 是否将上面设置应用到channel
	amqp_basic_qos((amqp_connection_state_t)m_pConn, m_nChannel, _prefetch_size, _prefetch_count, _global);

	amqp_bytes_t _queue = amqp_cstring_bytes(strQueueName.c_str());											// 消息队列名称
	amqp_bytes_t _tags = amqp_empty_bytes;																	// consumer标识
	int32_t _no_local = 0;																					// 1 不接收 0 接收
	int32_t _no_ack = 1;																					// 1 不应答 0 应答
	int32_t _exclusive = 0;																					// 1 连接不在队列自动删除 0 连接不在时不自动删除
	amqp_table_t _arguments = amqp_empty_table;																// 拓展参数
	amqp_basic_consume((amqp_connection_state_t)m_pConn, m_nChannel, _queue, _tags, _no_local, _no_ack, _exclusive, _arguments);
	if (!ErrorMsg(amqp_get_rpc_reply((amqp_connection_state_t)m_pConn)))
	{
		amqp_channel_close((amqp_connection_state_t)m_pConn, m_nChannel, AMQP_REPLY_SUCCESS);
		return false;
	}

	auto nCount = 1;
	amqp_envelope_t envelope;										// 接收消息结构体
	while (true)
	{
		amqp_maybe_release_buffers((amqp_connection_state_t)m_pConn);	// 释放channel与amqp_connection_state_t的连接
		int nFlag = 0;													// 保留值默认为0
		amqp_rpc_reply_t reply = amqp_consume_message((amqp_connection_state_t)m_pConn, &envelope, timeout, nFlag);

		if (/*ErrorMsg(amqp_consume_message((amqp_connection_state_t)m_pConn, &envelope, timeout, nFlag))*/
			reply.reply_type == AMQP_RESPONSE_NORMAL)
		{
			std::string strMsg((char*)envelope.message.body.bytes, envelope.message.body.len);
			myLogConsoleI("%s get message %s", __FUNCTION__, strMsg.c_str());
			myLogConsoleI("%s remain message number %d", __FUNCTION__, nCount++);

			// ack接收到的消息
			int32_t _multiple = 0;										// 1 批量处理，一次性ack所有小于delivery_tag的消息 0 不批量处理
			amqp_basic_ack((amqp_connection_state_t)m_pConn, m_nChannel, envelope.delivery_tag, _multiple);

			//// 拒绝接收到的消息
			//int32_t _requeue = 0;										// 1 被拒绝的重新入队列
			//amqp_basic_nack((amqp_connection_state_t)m_pConn, m_nChannel, envelope.delivery_tag, _multiple, _requeue);

			amqp_destroy_envelope(&envelope);																	// 销毁消息结构体
			amqp_channel_close((amqp_connection_state_t)m_pConn, m_nChannel, AMQP_REPLY_SUCCESS);				// 关闭连接通道
		}
		else
		{
			continue;
		}
	}
}
unsigned int __stdcall CRabbitMQ::HeartBeatFunc(LPVOID pParam)
{
	CRabbitMQ* mq = (CRabbitMQ*)pParam;
	mq->HeartBeatThread();

	return 0;
}
void CRabbitMQ::HeartBeatThread()
{
	myLogConsoleI("%s start heart beats...", __FUNCTION__);
	time_t tLastTick = time(NULL);
	while (m_bHeartBeatFlag)
	{
		time_t tNowTick = time(NULL);
		if (tNowTick - tLastTick >= 20)
		{
			amqp_frame_t heartbeat;
			heartbeat.channel = 0;
			heartbeat.frame_type = AMQP_FRAME_HEARTBEAT;

			CAutoLock lock(&m_csLock);
			if (m_pConn)
			{
				amqp_send_frame((amqp_connection_state_t)m_pConn, &heartbeat);
				myLogConsoleI("%s send heart beats...ticks %ld", __FUNCTION__, tNowTick);
			}

			tLastTick = tNowTick;
		}
		else
		{
			Sleep(500);
		}
	}
}
bool CRabbitMQ::AddQueue(const std::string strQueueName)
{
	CAutoLock lock(&m_csLock);
	m_vectQueueMap.push_back(strQueueName);

	return 0;
}
bool CRabbitMQ::StartConsume()
{
	if (NULL == m_pConn)
	{
		myLogConsoleI("%s m_pConn is null", __FUNCTION__);
		myLogFileI("%s m_pConn is null", __FUNCTION__);
		return false;
	}

	amqp_channel_open((amqp_connection_state_t)m_pConn, m_nChannel);										// 开启连接通道
	if (!ErrorMsg(amqp_get_rpc_reply((amqp_connection_state_t)m_pConn)))
	{
		amqp_channel_close((amqp_connection_state_t)m_pConn, m_nChannel, AMQP_REPLY_SUCCESS);
		return false;
	}

	CAutoLock lock(&m_csLock);
	for (auto i = 0; i < m_vectQueueMap.size(); i++)
	{
		int32_t _prefetch_size = 0;																				// 默认为0
		int16_t _prefetch_count = 1;																			// 表示rabbitmq不会同时给一个消费者推送多于_prefetch_count个消息，即如果有_prefetch_count和消息还没有ack，则该consumer将阻塞，知道有消息ack
		int32_t _global = 0;																					// 是否将上面设置应用到channel
		amqp_basic_qos((amqp_connection_state_t)m_pConn, m_nChannel, _prefetch_size, _prefetch_count, _global);	// 该api的参数设置会极大的影响到rabbitmq的性能，详情可参见:http://www.rabbitmq.com/blog/2012/05/11/some-queuing-theory-throughput-latency-and-bandwidth/

		amqp_bytes_t _queue = amqp_cstring_bytes(m_vectQueueMap.at(i).c_str());											// 消息队列名称
		amqp_bytes_t _tags = amqp_empty_bytes;																	// consumer标识
		int32_t _no_local = 0;																					// 1 不接收 0 接收
		int32_t _no_ack = 1;																					// 1 不应答 0 应答
		int32_t _exclusive = 0;																					// 1 连接不在队列自动删除 0 连接不在时不自动删除
		amqp_table_t _arguments = amqp_empty_table;																// 拓展参数
		amqp_basic_consume((amqp_connection_state_t)m_pConn, m_nChannel, _queue, _tags, _no_local, _no_ack, _exclusive, _arguments);
		if (!ErrorMsg(amqp_get_rpc_reply((amqp_connection_state_t)m_pConn)))
		{
			amqp_channel_close((amqp_connection_state_t)m_pConn, m_nChannel, AMQP_REPLY_SUCCESS);
			return false;
		}
	}

	if (NULL == m_hConsume)
	{
		m_bConsumeFlag = true;
		m_hConsume = (HANDLE)_beginthreadex(NULL, 0, ConsumeFunc, this, 0, &m_uiConsumeThreadId);
		Sleep(200);
	}

	amqp_channel_close((amqp_connection_state_t)m_pConn, m_nChannel, AMQP_REPLY_SUCCESS);
	return true;
}
unsigned int __stdcall CRabbitMQ::ConsumeFunc(LPVOID pParam)
{
	CRabbitMQ* mq = (CRabbitMQ*)pParam;
	mq->ConsumeThread();

	return 0;
}
void CRabbitMQ::ConsumeThread()
{
	timeval timeout;
	timeout.tv_usec = 0;
	timeout.tv_sec = 1;

	auto nCount = 1;
	amqp_envelope_t envelope;										// 接收消息结构体

	while (m_bConsumeFlag)
	{
		amqp_maybe_release_buffers((amqp_connection_state_t)m_pConn);	// 释放channel与amqp_connection_state_t的连接
		int nFlag = 0;													// 保留值默认为0
		amqp_rpc_reply_t reply = amqp_consume_message((amqp_connection_state_t)m_pConn, &envelope, &timeout, nFlag);

		if (reply.reply_type == AMQP_RESPONSE_NORMAL)
		{
			std::string strMsg((char*)envelope.message.body.bytes, envelope.message.body.len);
			myLogConsoleI("%s get message %s", __FUNCTION__, strMsg.c_str());
			myLogConsoleI("%s remain message number %d", __FUNCTION__, nCount++);

			// ack接收到的消息
			int32_t _multiple = 0;										// 1 批量处理，一次性ack所有小于delivery_tag的消息 0 不批量处理
			amqp_basic_ack((amqp_connection_state_t)m_pConn, m_nChannel, envelope.delivery_tag, _multiple);

			//// 拒绝接收到的消息
			//int32_t _requeue = 0;										// 1 被拒绝的重新入队列
			//amqp_basic_nack((amqp_connection_state_t)m_pConn, m_nChannel, envelope.delivery_tag, _multiple, _requeue);

			amqp_destroy_envelope(&envelope);																	// 销毁消息结构体
			amqp_channel_close((amqp_connection_state_t)m_pConn, m_nChannel, AMQP_REPLY_SUCCESS);				// 关闭连接通道
		}
		else
		{
			continue;
		}
	}
}
bool CRabbitMQ::ErrorMsg(amqp_rpc_reply_t x)
{
	switch (x.reply_type) 
	{
	case AMQP_RESPONSE_NORMAL:
		return true;
	case AMQP_RESPONSE_NONE:
		myLogConsoleI("%s missing RPC reply type!", __FUNCTION__);
		myLogFileI("%s missing RPC reply type!", __FUNCTION__);
		break;
	case AMQP_RESPONSE_LIBRARY_EXCEPTION:
		myLogConsoleI("%s %s", __FUNCTION__, amqp_error_string2(x.library_error));
		myLogFileI("%s %s", __FUNCTION__, amqp_error_string2(x.library_error));
		break;
	case AMQP_RESPONSE_SERVER_EXCEPTION:
		switch (x.reply.id) 
		{
			case AMQP_CONNECTION_CLOSE_METHOD:
			{
				amqp_connection_close_t *m = (amqp_connection_close_t *)x.reply.decoded;
				myLogConsoleI("%s server connection error %d", __FUNCTION__, m->reply_code, (int)m->reply_text.len);
				myLogFileI("%s server connection error %d", __FUNCTION__, m->reply_code, (int)m->reply_text.len);
			}
			break;
			case AMQP_CHANNEL_CLOSE_METHOD:
			{
				amqp_channel_close_t *m = (amqp_channel_close_t *)x.reply.decoded;
				myLogConsoleI("%s server channel error %d", __FUNCTION__, m->reply_code, (int)m->reply_text.len);
				myLogFileI("%s server channel error %d", __FUNCTION__, m->reply_code, (int)m->reply_text.len);
			}
			break;
			default:
				myLogConsoleI("%s unknown server error, method id 0x%X", __FUNCTION__, x.reply.id);
				myLogFileI("%s unknown server error, method id 0x%X", __FUNCTION__, x.reply.id);
			break;
		}
		break;
	}
	return false;
}

CRabbitMQ_Producer::CRabbitMQ_Producer()
{
	m_objRabbitMQ = new CRabbitMQ("localhost", 5672, "root", "root", 1000000);
	m_objRabbitMQ->ConnectRabbitServer();
}
CRabbitMQ_Producer::~CRabbitMQ_Producer()
{
	m_objRabbitMQ->Disconnect();
}

CRabbitMQ_Consumer::CRabbitMQ_Consumer()
{
	m_objRabbitMQ = new CRabbitMQ("localhost", 5672, "root", "root", 1000000);
	m_objRabbitMQ->ConnectRabbitServer();
}
CRabbitMQ_Consumer::~CRabbitMQ_Consumer()
{
	m_objRabbitMQ->Disconnect();
}

CQueueThread::CQueueThread(HANDLE hCompletionPort)
{
	m_hCompletionPort = hCompletionPort;
	memset(m_szBuffer, 0, sizeof(m_szBuffer));
}
CQueueThread::~CQueueThread()
{
	::CloseHandle(m_hCompletionPort);
	m_hCompletionPort = NULL;
	memset(m_szBuffer, 0, sizeof(m_szBuffer));
}
bool CQueueThread::Run()
{
	DWORD dwTrancferred = 0;
	OVERLAPPED* pOverlapped = NULL;
	CQueueService* pQueueService = NULL;

	if (::GetQueuedCompletionStatus(m_hCompletionPort, &dwTrancferred, (PULONG_PTR)&pQueueService, &pOverlapped, INFINITE))
	{
		/*if (NULL == pQueueService)
		{
			return false;
		}
		DataHead stuDataHead = { 0 };
		if (pQueueService->m_DataPool.GetData(stuDataHead, m_szBuffer, sizeof(m_szBuffer)))
		{
			pQueueService->OnRequest((void*)m_szBuffer);
		}*/
		myLogConsoleI("GetQueuedCompletionStatus");
	}

	return true;
}

CQueueService::CQueueService()
{
	m_hCompletionPort = NULL;
}
CQueueService::~CQueueService()
{
	StopService();
}
bool CQueueService::StartService()
{
	m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
	if (NULL == m_hCompletionPort)
	{
		myLogConsoleE("完成端口创建失败...");
		return false;
	}

	m_pQueueServiceThread = new CQueueThread(m_hCompletionPort);
	m_pQueueServiceThread->StartThread();

	return true;
}
bool CQueueService::StopService()
{
	if (NULL != m_hCompletionPort)
	{
		::PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL);
	}
	m_pQueueServiceThread->StopThread();

	if (NULL != m_hCompletionPort)
	{
		::CloseHandle(m_hCompletionPort);
		m_hCompletionPort = NULL;
	}

	return true;
}
bool CQueueService::AddToQueue(WORD wIdentifier, void* pData, WORD wDataLen)
{
	CAutoLock lock(&m_csLock);
	m_DataPool.AddData(wIdentifier, pData, wDataLen);
	::PostQueuedCompletionStatus(m_hCompletionPort, wDataLen, (ULONG_PTR)this, NULL);
	myLogConsoleI("PostQueuedCompletionStatus");

	return true;
}
bool CQueueService::OnRequest(void* pParam)
{
	
	return true;
}
void CQueueService::DoWorkLoop()
{
	
}