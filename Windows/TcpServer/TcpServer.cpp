#include <iostream>
#include <fstream>
#include "iocp.h"
#include "protocol.pb.h"

int main()
{
#if 1
	CIocpTcpServer * IocpServer = new CIocpTcpServer(CPS_FLAG_DEFAULT);
	if (!IocpServer || !IocpServer->Initialize("127.0.0.1", 8888, 32, 64, 4, 10000))
	{
		myLogConsoleE("server initialize failed");
	}
	char g = getchar();
	while ('q' == g)
	{
		IocpServer->Shutdown();
		break;
	}
#else
	const std::string strIp = "127.0.0.1";
	CIocpTcpClient* pClient = new CIocpTcpClient;
	if (!pClient->Create())
	{
		pClient->Destroy();
		myLogConsoleE("pClient->Create() failed");
		return 0;
	}
	if (!pClient->BeginConnect(strIp, 8888))
	{
		pClient->Destroy();
		myLogConsoleE("BeginConnect failed");
		return 0;
	}
	std::string str1 = "helloworld!";
	std::string str2 = "helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!ssssssssssssss!helloworld!";
	std::string str3 = "abcdefg";
	pClient->SendData(str1.c_str(), str1.size());
	pClient->SendData(str2.c_str(), str2.size());
	pClient->SendData(str3.c_str(), str3.size());
	pClient->SendData(str1.c_str(), str1.size());
	pClient->SendData(str3.c_str(), str3.size());
	Sleep(5000);
	pClient->DisconnectServer();
	Sleep(5000);
	pClient->BeginConnect(strIp, 8888);
	while (TRUE)
	{
		Sleep(1000);
		pClient->SendData(str1.c_str(), str1.size());
	}
	char c = getchar();
	pClient->Destroy();
#endif
	return 0;
}
