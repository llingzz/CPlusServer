#include <iostream>
#include <fstream>
#include "iocp.h"
#include "protocol.pb.h"

int main()
{
	CIocpTcpServer* IocpServer = new CIocpTcpServer(CPS_FLAG_DEFAULT);
	if (!IocpServer || !IocpServer->Initialize("127.0.0.1", 8888, 32, 64, 4, 10000))
	{
		myLogConsoleI("server initialize failed");
	}
	//const std::string strIp = "127.0.0.1";
	//IocpServer->ConnectOneServer(strIp, 8888);
	char g = getchar();
	while ('q' == g)
	{
		IocpServer->Shutdown();
		break;
	}
	return 0;
}