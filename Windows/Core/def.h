#pragma once

#define SAFE_DELETE(x) {if(x!=nullptr){delete x;x=nullptr;}}
#define SAFE_DELETE_ARRAY(x) {if(x!=nullptr){delete[] x;x=nullptr;}}
#define SAFE_RELEASE_SOCKET(x) {if(x!=INVALID_SOCKET){::closesocket(x);x=INVALID_SOCKET;}}
#define SAFE_RELEASE_HANDLE(x) {if(x!=NULL&&x!=INVALID_HANDLE_VALUE){::CloseHandle(x);x=INVALID_HANDLE_VALUE;}}

#define CPS_FLAG_DEFAULT 0x01
#define CPS_FLAG_MSG_HEAD 0x02

#define BASE_SOCKET_BUF_LEN 1024
#define MAX_SOCKET_BUF_LEN 8192
