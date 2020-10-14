#include <iostream>
#include <fstream>
#include "iocp.h"
#include "protocol.pb.h"

int main()
{
#if 0
	CIocpTcpServer* IocpServer = new CIocpTcpServer(CPS_FLAG_DEFAULT);
	if (!IocpServer || !IocpServer->Initialize("127.0.0.1", 8888, 32, 64, 4, 10000))
	{
		myLogConsoleI("server initialize failed");
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
	pClient->Create();
	pClient->BeginConnect(strIp, 8888);
	std::string str1 = "helloworld!";
	std::string str2 = "helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!helloworld!ssssssssssssss!helloworld!";
	std::string str3 = "abcdefg";
	pClient->SendData(str1.c_str(), str1.size());
	pClient->SendData(str1.c_str(), str1.size());
	pClient->SendData(str3.c_str(), str3.size());
	char g = getchar();
#endif
	return 0;
}