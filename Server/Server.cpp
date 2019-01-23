// Server.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

void TestTimer(void* pParam)
{
	std::cout << "Hello World!" << std::endl;
}
void CancelTest(void* pParam)
{
	std::cout << "Canceled!" << std::endl;
}

class testTask : public CTask
{
public:
	testTask(int nPriority = 0, int nTest = 0) :CTask(nPriority)
	{
		m_nTest = nTest;
	}
	~testTask()
	{

	}

	virtual void Run();

private:
	int m_nTest;
};
void testTask::Run()
{
	printf("执行任务%d\n", m_nTest);
}

int _tmain(int argc, _TCHAR* argv[])
{
#if 0
	CBaseServer GameServer = CBaseServer();
	//GetPrivateProfileString(_T("Server"), _T("ip"), _T(""), GameServer.m_szIp, _countof(GameServer.m_szIp), GameServer._getIniFile());
	if (FALSE == GameServer.Initialize())
	{
		//LOG_ERROR("Iniatialize Failed...");
	}
	char ch;
	do
	{
		ch = 'A';
		ch = toupper(ch);

		if ('S' == ch)
		{
			//GameServer.TestSend();
		}
		//ch = 'Q';

	} while ('Q' != ch);
#elif 0	//测试内存管理
	char test[] = "test";
	MESSAGE_HEAD stuMessageHead = { 0 };
	stuMessageHead.hSocket = 100;
	stuMessageHead.lSession = 100;
	stuMessageHead.lTokenID = 100;
	MESSAGE_CONTENT stuMessageContent = { 0 };
	stuMessageContent.nRequest = 10000;
	stuMessageContent.nDataLen = sizeof(test);
	stuMessageContent.pDataPtr = test;
	/*CBuffer myDataPool;
	myDataPool.Write((PBYTE)&stuMessageHead, sizeof(MESSAGE_HEAD));
	myDataPool.Write((PBYTE)&stuMessageContent, sizeof(MESSAGE_CONTENT));
	LPMESSAGE_HEAD lpHead = LPMESSAGE_HEAD(PBYTE(myDataPool.GetBuffer()));
	LPMESSAGE_CONTENT lpContent = LPMESSAGE_CONTENT(PBYTE(myDataPool.GetBuffer()+sizeof(MESSAGE_HEAD)));*/
	
	/*CBufferEx myDataPool;
	myDataPool.Write((PBYTE)"Test", sizeof("Test"));
	myDataPool.Write((PBYTE)"Test", sizeof("Test"));
	std::cout<<myDataPool.c_Bytes() + sizeof("Test")<<std::endl;*/
	CBufferEx myDataPool;
	myDataPool.Write((PBYTE)&stuMessageHead, sizeof(MESSAGE_HEAD));
	myDataPool.Write((PBYTE)&stuMessageContent, sizeof(MESSAGE_CONTENT));
	LPMESSAGE_HEAD lpHead = LPMESSAGE_HEAD(PBYTE(myDataPool.c_Bytes()));
	LPMESSAGE_CONTENT lpContent = LPMESSAGE_CONTENT(PBYTE(myDataPool.c_Bytes() + sizeof(MESSAGE_HEAD)));
#elif 0 //测试内存池
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
#elif 0 //测试消息队列
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
#elif 1	//测试自定义日志
	//CLog::GetInstance()->SetLogLevel(enINFO)->WriteLogFile("INFO %d", 1);
	LOG_INFO("INFO %d", 1);
	LOG_INFO("INFO %d", 2);
	LOG_INFO("INFO %d", 3);
	LOG_INFO("INFO %d", 4);
	//LOG_INFO("INFO %d %s", 5, "hello world!");
	//CLog::GetInstance()->SetLogLevel(enDEBUG)->WriteLogFile("DEBUG %d", 2);
	//LOG_DEBUG("DEBUG %d %s", 1, "DEBUG");
	//CLog::GetInstance()->SetLogLevel(enWARN)->WriteLogFile("WARN %d", 3);
	//CLog::GetInstance()->SetLogLevel(enTRACE)->WriteLogFile("TRACE %d", 4);
	//CLog::GetInstance()->SetLogLevel(enERROR)->WriteLogFile("ERROR %d", 5);
	//CLog::GetInstance()->SetLogLevel(enFATAL)->WriteLogFile("FATAL %d", 6);
#else
	cout << "Nothing Is Done!" << endl;
#endif
	getchar();

	return 0;
}