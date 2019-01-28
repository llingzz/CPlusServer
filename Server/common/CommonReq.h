#pragma once

#define TEST_CONNECTION		10000

//日志级别枚举
typedef enum enLogLevel{
	enDEFAULT = 0,
	enINFO,
	enDEBUG,
	enWARN,
	enTRACE,
	enERROR,
	enFATAL,
};
//文件日志快捷宏
#if	_DEBUG
#define		FILE_INFOS(fmt, ...)		CLog::GetInstance()->SetLogLevel(enINFO)->WriteLogFile(fmt, __VA_ARGS__)
#define		FILE_DEBUG(fmt, ...)		CLog::GetInstance()->SetLogLevel(enDEBUG)->WriteLogFile(fmt, __VA_ARGS__)
#define		FILE_WARNS(fmt, ...)		CLog::GetInstance()->SetLogLevel(enWARN)->WriteLogFile(fmt, __VA_ARGS__)
#define		FILE_TARCE(fmt, ...)		CLog::GetInstance()->SetLogLevel(enTRACE)->WriteLogFile(fmt, __VA_ARGS__)
#define		FILE_ERROR(fmt, ...)		CLog::GetInstance()->SetLogLevel(enERROR)->WriteLogFile(fmt, __VA_ARGS__)
#define		FILE_FATAL(fmt, ...)		CLog::GetInstance()->SetLogLevel(enFATAL)->WriteLogFile(fmt, __VA_ARGS__)
#else
#define		FILE_INFOS(fmt, ...)		CLog::GetInstance()->SetLogLevel(enINFO)->WriteLogFileEx(fmt, __VA_ARGS__)
#define		FILE_DEBUG(fmt, ...)		CLog::GetInstance()->SetLogLevel(enDEBUG)->WriteLogFileEx(fmt, __VA_ARGS__)
#define		FILE_WARNS(fmt, ...)		CLog::GetInstance()->SetLogLevel(enWARN)->WriteLogFileEx(fmt, __VA_ARGS__)
#define		FILE_TARCE(fmt, ...)		CLog::GetInstance()->SetLogLevel(enTRACE)->WriteLogFileEx(fmt, __VA_ARGS__)
#define		FILE_ERROR(fmt, ...)		CLog::GetInstance()->SetLogLevel(enERROR)->WriteLogFileEx(fmt, __VA_ARGS__)
#define		FILE_FATAL(fmt, ...)		CLog::GetInstance()->SetLogLevel(enFATAL)->WriteLogFileEx(fmt, __VA_ARGS__)
#endif
//控制台日志快捷宏
#define		CONSOLE_INFOS(fmt, ...)		CLog::GetInstance()->SetLogLevel(enINFO)->WriteLogConsole(fmt, __VA_ARGS__)
#define		CONSOLE_DEBUG(fmt, ...)		CLog::GetInstance()->SetLogLevel(enDEBUG)->WriteLogConsole(fmt, __VA_ARGS__)
#define		CONSOLE_WARNS(fmt, ...)		CLog::GetInstance()->SetLogLevel(enWARN)->WriteLogConsole(fmt, __VA_ARGS__)
#define		CONSOLE_TRACE(fmt, ...)		CLog::GetInstance()->SetLogLevel(enTRACE)->WriteLogConsole(fmt, __VA_ARGS__)
#define		CONSOLE_ERROR(fmt, ...)		CLog::GetInstance()->SetLogLevel(enERROR)->WriteLogConsole(fmt, __VA_ARGS__)
#define		CONSOLE_FATAL(fmt, ...)		CLog::GetInstance()->SetLogLevel(enFATAL)->WriteLogConsole(fmt, __VA_ARGS__)

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