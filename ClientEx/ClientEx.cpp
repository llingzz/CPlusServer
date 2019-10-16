// ClientEx.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

int _tmain(int argc, _TCHAR* argv[])
{
	CIocpClient* IocpClient = new CIocpClient;
	BOOL bRet = IocpClient->Create("127.0.0.1", 8888, 10, 10, 10000, 4, 0, 1);
	if (bRet)
	{
		PACKET_HEAD stuHead = { 0 };
		std::string str = "hello";
		stuHead.uiPacketNo = sizeof(PACKET_HEAD) + str.size() - sizeof(int);
		stuHead.uiMsgType = 1;
		stuHead.uiPacketLen = str.size();

		CONTEXT_HEAD lpHead = { 0 };
		REQUEST lpRequest;

		CBuffer myDataPool;
		myDataPool.Write((PBYTE)&stuHead, sizeof(PACKET_HEAD));
		myDataPool.Write((PBYTE)str.c_str(), str.size());

		lpRequest.m_pDataPtr = myDataPool.GetBuffer();
		lpRequest.m_nDataLen = myDataPool.GetBufferLen();
		//IocpClient->SendCast(0, &lpHead, &lpRequest, enRequest);
	}

	while(TRUE)
	{
		Sleep(1000);
	}

	IocpClient->Destroy();
	SAFE_DELETE(IocpClient);
	Sleep(5000);

	_CrtDumpMemoryLeaks();
	return 0;
}