#include "stdafx.h"

int DB_TestMySQL(_ConnectionPtr& connection, char* szSQL)
{
	auto nResult = 0;
	try{
		_RecordsetPtr pRecordset = NULL;
		TESTHR(pRecordset.CreateInstance(__uuidof(Recordset)));
		TESTHR(pRecordset->Open(szSQL, _variant_t((IDispatch *)connection, true), adOpenDynamic, adLockOptimistic, adCmdText));
		//执行更新等写操作时，注释
		//if (adStateOpen == pRecordset->GetState())
		//{
		//	while (VARIANT_FALSE == pRecordset->adoEOF)
		//	{
		//		//auto id = pRecordset->Fields->Item["id"]->Value.lVal;
		//		//printf("%d\n", id);

		//		pRecordset->MoveNext();
		//	}
		//}
		//TESTHR(pRecordset->Close());
		nResult = 1;
	}
	catch (_com_error e)
	{
		nResult = -1;
		printf(e.Description());
	}

	return nResult;
}