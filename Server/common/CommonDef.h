#pragma once

#include <WinSock2.h>
#include <MSWSock.h>
#pragma comment(lib,"ws2_32.lib")

#define				SAFE_DELETE(x)							{if(x!=NULL){delete x;x=NULL;}}
#define				SAFE_RELEASE_SOCKET(x)					{if(x!=INVALID_SOCKET){closesocket(x);x=INVALID_SOCKET;}}
#define				SAFE_RELEASE_HANDLE(x)					{if(x!=NULL&&x!=INVALID_HANDLE_VALUE){CloseHandle(x);x=NULL;}}

#define				MAX_WORKER_THREADS_PER_PROCESS			2			//每个核心对应几个工作线程
#define				MAX_MEANWHILE_POST_ACCEPT				1			//同时投递的Accept请求数
#define				BASE_DATA_BUF_SIZE						(1 * 1024)	//基本数据缓存区大小
#define				DATA_BUF_SIZE							(8 * 1024)	//数据缓冲区大小
#define				ACCEPTEX_BYTES_OFFSET_SIZE				16			//AcceptEx字节偏移量
#define				HEART_BEAT_WHEEL_SLOT					60			//心跳检测时间轮槽数
#define				TIME_TICK								1000		//1000ms
#define				IP_LENGTH								16			//IP字符串长度
#define				MAX_DISCONNECTION_TIMES					3			//心跳包最大缺失次数

//协议号
#define				PROTOCOL_HEART_PLUSE					10000		//心跳

//完成端口上投递IO操作类型
typedef enum tagOPE_TYPE{
	OPE_NULL,		//默认值
	OPE_ACCEPT,		//接收客户端连接
	OPE_RECV,		//接收数据
	OPE_SEND,		//发送数据
}OPE_TYPE;

//工作者线程
class CIOCPModule;
typedef struct _tagWORKER_THREAD{
	CIOCPModule*	pCIOCPModule;
	int				nThreadId;
}WORKER_THREAD, *LPWORKER_THREAD;

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

//心跳检测包
typedef struct _tagHEART_BEAT_DETECT{
	SOCKET			m_Socket;							//Socket
	unsigned int	m_uiDisconnectCount;				//心跳包缺收次数
	unsigned int	m_uiLastTick;
}HEART_BEAT_DETECT, *LPHEART_BEAT_DETECT;