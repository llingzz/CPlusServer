#pragma once

#define TEST_CONNECTION		10000

typedef struct _tagCONTEXT_HEAD{
	SOCKET	hSocket;
	LONG	lSession;
	BOOL	bNeedEcho;
	LONG	lTokenID;
	DWORD	dwFlags;
	int		nReserved[1];
}CONTEXT_HEAD, *LPCONTEXT_HEAD;

typedef struct _tagREQUEST_HEAD{
	INT		nRepeated;
	UINT	nRequest;
	int		nValue;
	int		nSubReq;
	WPARAM	wParam;
	LPARAM	lParam;
}REQUEST_HEAD, *LPREQUEST_HEAD;

class REQUEST{
public:
	REQUEST(){
		ZeroMemory(this, sizeof(REQUEST));
	}
	REQUEST(UINT nReq, void* pData, int nLen){
		ZeroMemory(this, sizeof(REQUEST));
		head.nRequest = nReq;
		pDataPtr = pData;
		nDataLen = nLen;
	}
	~REQUEST(){
	}
	REQUEST_HEAD head;
	int		nDataLen;
	VOID*	pDataPtr;
};
typedef REQUEST*  LPREQUEST;