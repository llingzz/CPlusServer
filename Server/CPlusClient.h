#pragma once

class CPlusClient : public CPlusServer{
public:
	CPlusClient();
	~CPlusClient();

public:
	virtual BOOL Create();
	virtual BOOL Destroy();

	virtual BOOL Connect();
	virtual BOOL SendRequest(SOCKET sClient, int nRequest, void* pDataPtr, int nDataLen);

	virtual BOOL OnReceiveData(SOCKET sSocket, void* pDataPtr, int nDataLen);
	virtual BOOL OnResponseData(SOCKET sSocket, void* pDataPtr, int nDataLen);
};