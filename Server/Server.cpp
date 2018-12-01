// Server.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#import "dll/msado15.dll" no_namespace rename("EOF","adoEOF") rename("BOF","adoBOF")

int _tmain(int argc, _TCHAR* argv[])
{
#if 0
	CBaseServer GameServer = CBaseServer();

	if (FALSE == GameServer.Initialize())
	{
		//LOG_ERROR("Iniatialize Failed...");
	}
	char ch;
	do
	{
		ch = 'S';
		ch = toupper(ch);

		if ('S' == ch)
		{
			GameServer.TestSend();
		}
		ch = 'Q';

	} while ('Q' != ch);
#elif 0	//测试内存池
	CBuffer myDataPool;
	myDataPool.Write((PBYTE)"test", 5);
	cout<<myDataPool.GetBuffer()<<endl;
#elif 0	//测试Redis连接
	CRedisManager Manager("127.0.0.1",6379,0);
	Manager.ConnectServer("127.0.0.1", 6379, "8767626", 0);
	Manager.PingServer();
#elif 0	//测试MySQL连接
	::CoInitialize(NULL);
	HRESULT hr;
	_ConnectionPtr _Connection;
	_RecordsetPtr _RecordSet;

	try
	{
		hr = _Connection.CreateInstance("ADODB.Connection");
		if (SUCCEEDED(hr))
		{
			_Connection->ConnectionTimeout = 600;
			_Connection->CommandTimeout = 120;

			_Connection->Open("DSN=MySql;Server=localhost;Database=mygamedb", "root", "root", adModeUnknown);

			_bstr_t sql;
			sql = "Insert into user values(88,\'luoling\')";
			_RecordSet.CreateInstance(__uuidof(Recordset));
			_RecordSet->Open(sql, _Connection.GetInterfacePtr(), adOpenDynamic, adLockOptimistic, adCmdText);
			while (!_RecordSet->adoEOF)
			{
				auto name = (LPCTSTR)(_bstr_t)_RecordSet->GetCollect("Name");
				//std::cout << name << std::endl;
				printf("name : %s\n", name);
				_RecordSet->MoveNext();
			}
		}
	}
	catch (_com_error e)
	{

	}

	getchar();
#else
	cout << "Nothing Is Done!" << endl;
#endif
	getchar();

	return 0;
}

