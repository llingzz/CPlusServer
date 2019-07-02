// Server.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include <winhttp.h>
#pragma comment(lib, "Winhttp.lib")

void TestTimer(void* pParam)
{
	std::cout << "Hello World!" << std::endl;
}
void CancelTest(void* pParam)
{
	std::cout << "Canceled!" << std::endl;
}

BOOL testConsume(LPMQ_MESSAGE pMessage)
{
	myLogConsoleI("consumed message with message id:%d ", pMessage->nMessageID);
	return TRUE;
}

int _tmain(int argc, _TCHAR* argv[])
{
#if 0	//服务器启动
	CBaseServer GameServer = CBaseServer();
	if (FALSE == GameServer.Initialize())
	{
		FILE_ERROR("%s iniatialize failed...", __FUNCTION__);
		CONSOLE_ERROR("%s iniatialize failed...", __FUNCTION__);
	}
	char ch;
	do
	{
		ch = 'A';
		ch = toupper(ch);

	} while ('Q' != ch);
	GameServer.Shutdown();
#elif 0
	CPlusServer GameServer = CPlusServer();
	if (FALSE == GameServer.Initialize())
	{
		myLogFileE("%s gameserver iniatialize failed...", __FUNCTION__);
		myLogConsoleE("%s gameserver iniatialize failed...", __FUNCTION__);
	}
	char ch;
	do
	{
		ch = 'A';
		ch = toupper(ch);
	} while ('Q' != ch);
	GameServer.Shutdown();
#elif 1
	CIocpServer IocpServer = CIocpServer();
	if (!IocpServer.Initialize(_T("127.0.0.1"), 8888, 10, 20, 10, 10, 10, 10, 0, 10))
	{
		myLogConsoleI("server initialize failed");
	}
	char ch;
	do
	{
		ch = 'A';
		ch = toupper(ch);
	} while ('Q' != ch);
	IocpServer.Shutdown();
#elif 0
	CIocpWorker worker = CIocpWorker();
	worker.BeginWorkerPool(4, 0);
	worker.PutRequestToQueue(0, 1, "", "");
	Sleep(10000);
	worker.EndWorkerPool();
#elif 0
	CIocpWorker worker = CIocpWorker();
	worker.BeginWorkerPool(10, 0);
	worker.PutRequestToQueue(0, 10, "1234", "5678");
	worker.PutRequestToQueue(0, 10, "1234", "5678");
	worker.PutRequestToQueue(0, 10, "1234", "5678");
	worker.PutRequestToQueue(0, 10, "1234", "5678");
	worker.PutRequestToQueue(0, 10, "1234", "5678");
	worker.PutRequestToQueue(0, 10, "1234", "5678");
	worker.PutRequestToQueue(0, 10, "1234", "5678");
	worker.EndWorkerPool();
#elif 0	//测试内存池
	char test1[] = "test1";
	char test2[] = "test2";
	MESSAGE_HEAD stuMessageHead = { 0 };
	stuMessageHead.hSocket = 100;
	stuMessageHead.lSession = 100;
	stuMessageHead.lTokenID = 100;
	MESSAGE_CONTENT stuMessageContent = { 0 };
	stuMessageContent.nRequest = 10000;
	stuMessageContent.nDataLen = sizeof(test1);
	stuMessageContent.pDataPtr = test1;

	CBuffer buffer;
	buffer.Write((PBYTE)&test1, sizeof(test1));
	buffer.Write((PBYTE)&stuMessageHead, sizeof(stuMessageHead));
	buffer.Write((PBYTE)&stuMessageContent, sizeof(stuMessageContent));
	PBYTE pTemp1 = new BYTE[1024];
	PBYTE pTemp2 = new BYTE[1024];
	PBYTE pTemp3 = new BYTE[1024];
	buffer.Read(pTemp1, sizeof(test1));
	buffer.Read(pTemp2, sizeof(MESSAGE_HEAD));
	LPMESSAGE_HEAD lpHead = LPMESSAGE_HEAD(PBYTE(pTemp2));
	buffer.Read(pTemp3, sizeof(MESSAGE_CONTENT));
	LPMESSAGE_CONTENT lpContent = LPMESSAGE_CONTENT(PBYTE(pTemp3));
	int len1 = sizeof(test1);
	//int len2 = sizeof(stuMessageHead);
	//int len3 = sizeof(stuMessageContent);
	//PBYTE pRes = buffer.GetBuffer();
	//LPMESSAGE_HEAD lpHead = LPMESSAGE_HEAD(PBYTE(pRes + sizeof(test1)));
	//LPMESSAGE_CONTENT lpContent = LPMESSAGE_CONTENT(PBYTE(pRes + sizeof(test1) + sizeof(MESSAGE_HEAD)));

	/*CBufferEx myDataPool;
	myDataPool.Write((PBYTE)"Test", sizeof("Test"));
	myDataPool.Write((PBYTE)"Test", sizeof("Test"));
	std::cout<<myDataPool.c_Bytes() + sizeof("Test")<<std::endl;*/
	/*CBufferEx myDataPool;
	myDataPool.Write((PBYTE)&stuMessageHead, sizeof(MESSAGE_HEAD));
	myDataPool.Write((PBYTE)&stuMessageContent, sizeof(MESSAGE_CONTENT));
	LPMESSAGE_HEAD lpHead = LPMESSAGE_HEAD(PBYTE(myDataPool.c_Bytes()));
	LPMESSAGE_CONTENT lpContent = LPMESSAGE_CONTENT(PBYTE(myDataPool.c_Bytes() + sizeof(MESSAGE_HEAD)));*/
#elif 0 //测试环形缓冲区
	char writechar[8] = { 0 };
	memcpy(writechar, "hello", sizeof(writechar));
	CRingBuffer Buffer(16);
	Buffer.Write(writechar, sizeof(writechar));
	char readchar[4] = { 0 };
	myLogConsoleI("the length is %d", Buffer.GetLength());
	Buffer.Read(readchar, sizeof(readchar));
	myLogConsoleI("read data:%s ",readchar);
	char readchar1[8] = { 0 };
	Buffer.Read(readchar1, sizeof(readchar1));
	myLogConsoleI("read data:%s ",readchar);
#elif 0	//测试Redis连接
	CRedisManager Manager("127.0.0.1",6379,0);
	Manager.ConnectServer("127.0.0.1", 6379, "8767626", 0);
	Manager.PingServer();
#elif 0	//测试MySQL连接
	_ConnectionPtr connection = CMySQLManager::GetInstance()->GetConnection();
	//MYSQL_TestConnection(connection);
	//DB_TestMySQL(connection,"select id from test;");
	//测试执行写数据操作时事务性回滚
	auto nResult = MYSQL_TestConnection(connection);
	if(nResult < 0)
	{
		printf("database access failed...\n");
		return 0;
	}
	//检查数据库连接
	nResult = MYSQL_BeginTrans(connection);
	if (nResult <= 0)
	{
		printf("database access failed...\n");
	}
	nResult = DB_TestMySQL(connection, "insert into test values(999,\'insert\')");
	if (nResult <= 0)
	{
		printf("action invalid...rollback...\n");
		//事务回滚
		MYSQL_RollbackTrans(connection);
		return 0;
	}
	//提交事务操作
	MYSQL_CommitTrans(connection);
#elif 0	//测试线程池
	CThreadPool* pThreadPool = new CThreadPool(20, 10, 5);
	pThreadPool->Start();

	for (auto i = 0; i < 20; i++)
	{
		testTask* test = new testTask(0, i);
		pThreadPool->PushTask(test, NULL);
	}

	pThreadPool->Stop();
#elif 0	//测试定时器
	TimeWheel timer;
	timer.SetTimer(10000, TestTimer, NULL, CancelTest);
	//TimeWheelNode* pNode = timer.SetTimer(10000, TestTimer, NULL, CancelTest);
	//timer.CancelTimer(pNode->nTimerID);
#elif 0 //测试消息队列MQ_Manager
	MQ_Manager* pManager = new MQ_Manager;
	MQ_Publisher* pPublisher = new MQ_Publisher;
	MQ_Subscriber* pSubscriber1 = new MQ_Subscriber;
	MQ_Subscriber* pSubscriber2 = new MQ_Subscriber;

	MQ_MESSAGE stuMQMessage = { 0 };
	stuMQMessage.nMessageID = 1000;
	stuMQMessage.nMessageType = 1;
	strcpy(stuMQMessage.szMessageContent, "hello world!");

	pPublisher->PublishMQ(pManager, stuMQMessage.nMessageID, &stuMQMessage);
	pSubscriber1->SubscribMQ(pManager, stuMQMessage.nMessageID);
	pSubscriber2->SubscribMQ(pManager, stuMQMessage.nMessageID);
	pSubscriber2->UnSubscribMQ(pManager, stuMQMessage.nMessageID);

	MQ_MESSAGE stuMQMessage1 = { 0 };
	stuMQMessage1.nMessageID = 2000;
	stuMQMessage1.nMessageType = 2;
	strcpy(stuMQMessage1.szMessageContent, "hello world! hello world!");
	pPublisher->PublishMQ(pManager, stuMQMessage1.nMessageID, &stuMQMessage1);
	pSubscriber2->SubscribMQ(pManager, stuMQMessage1.nMessageID);

	//delete pManager;
	//delete pPublisher;
	//delete pSubscriber1;
	//delete pSubscriber2;
#elif 0 //测试消息队列MQ_ManagerEx
	MQ_MESSAGE stuMQMessage = { 0 };
	stuMQMessage.nMessageID = 1000;
	stuMQMessage.nMessageType = 1;
	strcpy(stuMQMessage.szMessageContent, "hello world!");
	while (1)
	{
		CMessageQueueEx::GetInstance()->SetCallbackFunc(testConsume)->Produce(&stuMQMessage);
		myLogConsoleI("produced message with message id:%d ", stuMQMessage.nMessageID);
		stuMQMessage.nMessageID++;
	}
	
	//delete pManager;
#elif 0	//测试自定义日志
	CLog::GetInstance()->SetLogLevel(enINFO)->WriteLogFile("INFO %d", 1);
	myLogFileI("INFO %d", 1);
	myLogFileI("INFO %d", 2);
	myLogFileI("INFO %d", 3);
	myLogFileI("INFO %d", 4);
	CLog::GetInstance()->SetLogLevel(enDEBUG)->WriteLogFile("DEBUG %d", 2);
	CLog::GetInstance()->SetLogLevel(enWARN)->WriteLogFile("WARN %d", 3);
	CLog::GetInstance()->SetLogLevel(enTRACE)->WriteLogFile("TRACE %d", 4);
	CLog::GetInstance()->SetLogLevel(enERROR)->WriteLogFile("ERROR %d", 5);
	CLog::GetInstance()->SetLogLevel(enFATAL)->WriteLogFile("FATAL %d", 6);
	CLog::GetInstance()->SetLogLevel(enINFO)->WriteLogFileEx("test %d", 1);
#elif 0 //测试JsonCpp
	std::string str = "{\"string\": \"string\",\"int\": 100,\"bool\": \"True\"}";
	char* strStudent = "{ \"name\" : \"cuishihao\", \"age\" : 28, \"major\" : \"cs\" }";
	Json::Reader reader;
	Json::Value root;
	if (reader.parse(str, root))
	{
		std::string strString = root["string"].asString();
		int intNum = root["int"].asInt();

		myLogConsoleI("string %s int %d", strString.c_str(), intNum);
	}
#elif 0 // 测试Rabbitmq
	myLogConsoleI("start CRabbitMQ_Publisher...");
	CRabbitMQ_Producer* mq_producer = new CRabbitMQ_Producer();
	mq_producer->SendMsg("", "", "", "Test");
#elif 0
	myLogConsoleI("start CRabbitMQ_Consumer...");
	CRabbitMQ_Consumer* mq_consumer = new CRabbitMQ_Consumer();
	mq_consumer->SetRouteFilter("queueTest","routekeyTest.test",NULL,NULL);
	mq_consumer->StartConsume();
#elif 0
	/* 接收超时时间 */
	timeval timeout;
	timeout.tv_usec = 0;
	timeout.tv_sec = 2;
	/* 变量声明 */
	//std::string strExchange = "MyExchange";
	//std::string strRoutekey = "routekeyTest.test.test";
	//std::string strQueuename = "MyQueue_1";
	CRabbitMQ *m_objRabbitMQ = new CRabbitMQ("localhost", 5672, "root", "root", 1000000);
	m_objRabbitMQ->ConnectRabbitServer();
	/* 声明交换机 */
	//m_objRabbitMQ->ExchangeDeclare("MyExchange_direct", "direct");
	//m_objRabbitMQ->ExchangeDeclare("MyExchange_fanout", "fanout");
	//m_objRabbitMQ->ExchangeDeclare("MyExchange_topic", "topic");
	/* 声明队列 */
	//m_objRabbitMQ->QueueDeclare("MyQueue_direct_1");
	//m_objRabbitMQ->QueueDeclare("MyQueue_fanout_1");
	//m_objRabbitMQ->QueueDeclare("MyQueue_topic_1");
	/* 绑定交换机与队列 */
	//m_objRabbitMQ->QueueBind("MyQueue_direct_1", "MyExchange_direct", "test.direct.*");
	//m_objRabbitMQ->QueueBind("MyQueue_fanout_1", "MyExchange_fanout", "test.fanout.*");
	//m_objRabbitMQ->QueueBind("MyQueue_topic_1", "MyExchange_topic", "test.topic.*");
	m_objRabbitMQ->QueueBind("MyQueue_topic_1", "MyExchange_topic", "test.topic.#");
	/* 取消交换机与队列的绑定 */
	//m_objRabbitMQ->QueueUnbind("MyQueue_direct_1", "MyExchange_direct", "test.direct");
	/* 发布消息 */
	//m_objRabbitMQ->SendMsg("MyExchange_direct", "MyQueue_direct_1", "test.direct.*", "(SendToExchange:MyQueue_direct_1 SendToQueue:MyExchange_direct) with RouteKey:test.direct");
	//m_objRabbitMQ->SendMsg("MyExchange_fanout", "MyQueue_fanout_1", "test.fanout.*", "(SendToExchange:MyQueue_fanout_1 SendToQueue:MyExchange_fanout) with RouteKey:test.fanout.*");
	//m_objRabbitMQ->SendMsg("MyExchange_topic", "MyQueue_topic_1", "test.topic.test", "(SendToExchange:MyExchange_topic SendToQueue:MyExchange_fanout) with RouteKey:test.fanout.test");
	for (auto i = 0; i < 100; i++)
	{
		m_objRabbitMQ->SendMsg("MyExchange_topic", "MyQueue_topic_1", "test.topic.test.test", "(SendToExchange:MyExchange_topic SendToQueue:MyQueue_topic_1) with RouteKey:test.topic.test.test");
	}
	/* 接收消息 */
	//m_objRabbitMQ->RecvMsg("MyQueue_direct_1", &timeout);
	//m_objRabbitMQ->RecvMsg("MyQueue_fanout_1", &timeout);
	//m_objRabbitMQ->RecvMsg("MyQueue_topic_1", &timeout);
	//m_objRabbitMQ->RecvMsg("MyQueue_topic_1", &timeout);
	/* 线程处理接收消息 */
	m_objRabbitMQ->AddQueue("MyQueue_topic_1");
	m_objRabbitMQ->StartConsume();
#elif 0 // 测试内存池
	DataHead stuData = { 0 };
	BUFFER_INFO stuBuffer = { 0 };
	char szBuffer[] = { 0 };
	CBufferPool myBufferPool;
	myBufferPool.AddData(10000, "test", sizeof("test"));
	myBufferPool.AddData(10000, "sssstssssest", sizeof("sssstssssest"));
	myBufferPool.GetData(stuData, szBuffer, 20);
	myBufferPool.AddData(10000, "sssstssssest", sizeof("sssstssssest"));
	myBufferPool.AddData(10000, "sssstssssest", sizeof("sssstssssest"));
	myBufferPool.AddData(10000, "sssstssssest", sizeof("sssstssssest"));
	myBufferPool.GetData(stuData, szBuffer, 20);
	myBufferPool.AddData(10000, "sssstssssest", sizeof("sssstssssest"));
	myBufferPool.AddData(10000, "sssstssssest", sizeof("sssstssssest"));
	myBufferPool.GetData(stuData, szBuffer, 20);
	myBufferPool.GetData(stuData, szBuffer, 20);
	myBufferPool.GetBufferPoolInfo(stuBuffer);
	getchar();
#elif 0 // 测试队列CQueueService
	myLogConsoleI("测试队列CQueueService");
	WSADATA wsaData;
	::WSAStartup(MAKEWORD(2, 2), &wsaData);

	CQueueService objQueueService = CQueueService();
	objQueueService.StartService();
	objQueueService.AddToQueue(10000, "hello world!", sizeof("hello world!"));
	objQueueService.AddToQueue(10000, "hello world!", sizeof("hello world!"));
	objQueueService.AddToQueue(10000, "hello world!", sizeof("hello world!"));
	objQueueService.StopService();

	WSACleanup();
#elif 0
	CIocpWorker worker = CIocpWorker();
	worker.BeginWorkerPool(10, 5);
	worker.PutRequestToQueue(1000, 1000, "hello", "world");
	//worker.EndWorkerPool();
#else
#endif
	getchar();
	return 0;
}