#pragma once

#include <WinSock2.h>
#include <MSWSock.h>
#pragma comment(lib,"ws2_32.lib")
#include <dbghelp.h > 
#pragma comment(lib, "dbghelp.lib")

#define				SERVER_NAME								"CPlusServer"

#define				SAFE_DELETE(x)							{if(x!=nullptr){delete x;x=nullptr;}}
#define				SAFE_DELETE_ARRAY(x)					{if(x!=nullptr){delete[] x;x=nullptr;}}
#define				SAFE_RELEASE_SOCKET(x)					{if(x!=INVALID_SOCKET){::closesocket(x);x=INVALID_SOCKET;}}
#define				SAFE_RELEASE_HANDLE(x)					{if(x!=NULL&&x!=INVALID_HANDLE_VALUE){::CloseHandle(x);x=INVALID_HANDLE_VALUE;}}

#define				MAX_WORKER_THREAD_NUMBER				32					//最多工作线程数
#define				ADD_ACCEPT_COUNT						32					//Accept投递不足时添加的Accept投递数
#define				MAX_WORKER_THREADS_PER_PROCESS			2					//每个核心对应几个工作线程
#define				MAX_MEANWHILE_POST_ACCEPT				1					//同时投递的Accept请求数
#define				BASE_CBUFFER_SIZE						1024				//内存池基本数据缓存区大小
#define				BASE_DATA_BUF_SIZE						(1 * 1024)			//基本数据缓存区大小
#define				DATA_BUF_SIZE							(8 * 1024)			//数据缓冲区大小
#define				ACCEPTEX_BYTES_OFFSET_SIZE				16					//AcceptEx字节偏移量
#define				HEART_BEAT_WHEEL_SLOT					60					//心跳检测时间轮槽数
#define				TIME_TICK								1000				//1000ms
#define				IP_LENGTH								16					//IP字符串长度
#define				MAX_DISCONNECTION_TIMES					3					//心跳包最大缺失次数
#define				MAX_PLUSE_INTEVAL						30					//心跳接受最大间隔数

//自定义内核消息
#define				WM_DATA_TO_SEND							(WM_USER + 1000)
#define				WM_DATA_TO_RECV							(WM_USER + 1001)

// iocp底层协议
#define				BASE_SOCKET_CONNECT						0					// 客户端连接
#define				BASE_SOCKET_CLOSE						1					// 客户端断开
#define				BASE_SOCKET_READ						2					// socket读操作
#define				BASE_SOCKET_WRITE						3					// socket写操作

//错误/返回码
#define				THREAD_EXIT								0					// 线程退出

//协议号
#define				PROTOCOL_CLIENT_PLUSE					10000				//心跳

//日志级别枚举
typedef enum enLogLevel {
	enDEFAULT = 0,
	enINFO,
	enDEBUG,
	enWARN,
	enTRACE,
	enERROR,
	enFATAL,
};

//日志宏定义
#if _DEBUG
//文件日志快捷宏
#define		myLogFileI(fmt, ...)		CLog::GetInstance()->SetLogLevel(enINFO)->WriteLogFile(fmt, __VA_ARGS__)
#define		myLogFileD(fmt, ...)		CLog::GetInstance()->SetLogLevel(enDEBUG)->WriteLogFile(fmt, __VA_ARGS__)
#define		myLogFileW(fmt, ...)		CLog::GetInstance()->SetLogLevel(enWARN)->WriteLogFile(fmt, __VA_ARGS__)
#define		myLogFileT(fmt, ...)		CLog::GetInstance()->SetLogLevel(enTRACE)->WriteLogFile(fmt, __VA_ARGS__)
#define		myLogFileE(fmt, ...)		CLog::GetInstance()->SetLogLevel(enERROR)->WriteLogFile(fmt, __VA_ARGS__)
#define		myLogFileF(fmt, ...)		CLog::GetInstance()->SetLogLevel(enFATAL)->WriteLogFile(fmt, __VA_ARGS__)
//控制台日志快捷宏
#define		myLogConsoleI(fmt, ...)		CLog::GetInstance()->SetLogLevel(enINFO)->WriteLogConsole(fmt, __VA_ARGS__)
#define		myLogConsoleD(fmt, ...)		CLog::GetInstance()->SetLogLevel(enDEBUG)->WriteLogConsole(fmt, __VA_ARGS__)
#define		myLogConsoleW(fmt, ...)		CLog::GetInstance()->SetLogLevel(enWARN)->WriteLogConsole(fmt, __VA_ARGS__)
#define		myLogConsoleT(fmt, ...)		CLog::GetInstance()->SetLogLevel(enTRACE)->WriteLogConsole(fmt, __VA_ARGS__)
#define		myLogConsoleE(fmt, ...)		CLog::GetInstance()->SetLogLevel(enERROR)->WriteLogConsole(fmt, __VA_ARGS__)
#define		myLogConsoleF(fmt, ...)		CLog::GetInstance()->SetLogLevel(enFATAL)->WriteLogConsole(fmt, __VA_ARGS__)
#else
//文件日志快捷宏
#define		myLogFileI(fmt, ...)		CLog::GetInstance()->SetLogLevel(enINFO)->WriteLogFileEx(fmt, __VA_ARGS__)
#define		myLogFileD(fmt, ...)		CLog::GetInstance()->SetLogLevel(enDEBUG)->WriteLogFileEx(fmt, __VA_ARGS__)
#define		myLogFileW(fmt, ...)		CLog::GetInstance()->SetLogLevel(enWARN)->WriteLogFileEx(fmt, __VA_ARGS__)
#define		myLogFileT(fmt, ...)		CLog::GetInstance()->SetLogLevel(enTRACE)->WriteLogFileEx(fmt, __VA_ARGS__)
#define		myLogFileE(fmt, ...)		CLog::GetInstance()->SetLogLevel(enERROR)->WriteLogFileEx(fmt, __VA_ARGS__)
#define		myLogFileF(fmt, ...)		CLog::GetInstance()->SetLogLevel(enFATAL)->WriteLogFileEx(fmt, __VA_ARGS__)
#endif

//完成端口上投递IO操作类型
typedef enum tagOPE_TYPE{
	OPE_NULL,		//默认值
	OPE_ACCEPT,		//接收客户端连接
	OPE_RECV,		//接收数据
	OPE_SEND,		//发送数据
}OPE_TYPE;

//工作者线程
class CPlusServer;
typedef struct _tagWORKER_THREAD_PARAM{
	CPlusServer*	pServer;
	int				nThreadId;
}WORKER_THREAD_PARAM, *LPWORKER_THREAD_PARAM;

//IO操作上下文（每一个重叠操作的参数）
typedef struct _tagIO_CONTEXT{
	OVERLAPPED		m_Overlapped;						//重叠操作结构
	SOCKET			m_Socket;							//IO操作对应Socket
	WSABUF			m_WsaBuf;							//重叠操作参数缓存区
	CHAR			m_szBuffer[DATA_BUF_SIZE];			//WSABUF缓冲区
	OPE_TYPE		m_OpeType;							//当前IO操作类型
}IO_CONTEXT, *LPIO_CONTEXT;

//SOCKET数据结构体
typedef struct _tagSOCKET_CONTEXT{
	SOCKET			m_Socket;							//Socket
	sockaddr_in     m_SockAddrIn;						//该Socket地址信息
	std::vector<LPIO_CONTEXT> m_vectIoContext;			//该Socket所有的IO上下文操作
}SOCKET_CONTEXT, *LPSOCKET_CONTEXT;

class CIoContext{
public:
	CIoContext(){
		ZeroMemory(&m_Overlapped, sizeof(OVERLAPPED));
		m_Socket = INVALID_SOCKET;
		ZeroMemory(&m_szBuffer, DATA_BUF_SIZE);
		m_WsaBuf.len = sizeof(m_szBuffer);
		m_WsaBuf.buf = m_szBuffer;
		m_OpeType = OPE_NULL;
	}
	~CIoContext(){
		ZeroMemory(&m_Overlapped, sizeof(OVERLAPPED));
		m_Socket = INVALID_SOCKET;
		ZeroMemory(&m_szBuffer, DATA_BUF_SIZE);
		m_WsaBuf.len = sizeof(m_szBuffer);
		m_WsaBuf.buf = m_szBuffer;
		m_OpeType = OPE_NULL;
	}
public:
	OVERLAPPED	m_Overlapped;
	SOCKET		m_Socket;
	CHAR		m_szBuffer[DATA_BUF_SIZE];
	WSABUF		m_WsaBuf;
	OPE_TYPE	m_OpeType;
};
typedef CIoContext* LPCIoContext;

class CClientContext{
public:
	CClientContext(){
		m_Socket = INVALID_SOCKET;
		ZeroMemory(&m_SockAddrIn, sizeof(sockaddr_in));
		m_vectIoContext.clear();
	}
	~CClientContext(){
		m_Socket = INVALID_SOCKET;
		ZeroMemory(&m_SockAddrIn, sizeof(sockaddr_in));
		m_vectIoContext.clear();
	}
public:
	SOCKET					m_Socket;
	sockaddr_in				m_SockAddrIn;
	std::vector<LPCIoContext> m_vectIoContext;
};
typedef CClientContext* LPClientContext;

typedef struct _tagMESSAGE_HEAD{
	SOCKET	hSocket;
	LONG	lSession;
	LONG	lTokenID;
}MESSAGE_HEAD, *LPMESSAGE_HEAD;

typedef struct _tagMESSAGE_CONTENT{
	UINT	nRequest;
	int		nDataLen;
	void*	pDataPtr;
}MESSAGE_CONTENT, *LPMESSAGE_CONTENT;

//心跳检测包
typedef struct _tagHEART_BEAT_DETECT{
	SOCKET			m_Socket;							//Socket
	unsigned int	m_uiDisconnectCount;				//心跳包缺收次数
	unsigned int	m_uiLastTick;
}HEART_BEAT_DETECT, *LPHEART_BEAT_DETECT;

typedef struct _tagPLUSE_PACKAGE{
	SOCKET			m_Socket;
	time_t			m_tLastTick;
}PLUSE_PACKAGE, *LPPLUSE_PACKAGE;

///////////////////////////////////////////////////////////////////////////
// 新定义区

// 线程参数
class CThread;
typedef struct _tagThreadParam {
	bool bSuccess;
	HANDLE hEventHandle;
	CThread* pThread;
}ThreadParam, *LPThreadParam;

// 数据包头
typedef struct _tagDataHead {
	WORD wIdentifier;
	WORD wDataSize;
}DataHead, *LPDataHead;

// 网络请求上下文头
typedef struct _tagContextHead{
	SOCKET	hSocket;
	LONG	lToken;
}ContextHead, *LPContextHead;
///////////////////////////////////////////////////////////////////////////