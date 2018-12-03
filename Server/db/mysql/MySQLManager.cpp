#include "stdafx.h"

CMySQLManager::CMySQLManager()
{
	_connect();
}
CMySQLManager::~CMySQLManager()
{
	_disConnect();
}

void CMySQLManager::_connect()
{
	//初始化OLE/COM库环境
	::CoInitialize(NULL);
	HRESULT hr;

	try{
		//创建Connection对象
		hr = m_pConnection.CreateInstance("ADODB.Connection");
		if (SUCCEEDED(hr))
		{
			m_pConnection->ConnectionTimeout = 600;
			m_pConnection->CommandTimeout = 120;
			//m_pConnection->Open("DSN=MySQL;Server=127.0.0.1;Database=test", "root", "root", adModeUnknown);
			m_pConnection->Open("DRIVER={MySQL ODBC 8.0 Unicode Driver};Server=127.0.0.1;Database=test", "root", "root", adModeUnknown);
		}
	}
	catch (_com_error e)
	{
		printf(e.Description());
	}
}

void CMySQLManager::_disConnect()
{
	//关闭连接与记录集
	if (NULL != m_pRecordset)
	{
		m_pRecordset->Close();
	}
	m_pConnection->Close();

	//释放OLE/COM环境
	::CoUninitialize();
}

_ConnectionPtr& CMySQLManager::GetConnection()
{
	return m_pConnection;
}
_RecordsetPtr& CMySQLManager::GetRecordset()
{
	return m_pRecordset;
}