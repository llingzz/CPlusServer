#include <iostream>
#include "HttpServer.h"

int main()
{
	CHttpServer* pHttpServer = new CHttpServer;
	if (!pHttpServer || !pHttpServer->Initialize("127.0.0.1", 8080, 32, 64, 4, 10000))
	{
		myLogConsoleI("server initialize failed");
	}
	char g = getchar();
	while ('q' == g)
	{
		pHttpServer->Shutdown();
		break;
	}
	return 0;
}
