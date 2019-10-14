// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define new  new(_CLIENT_BLOCK, __FILE__, __LINE__)
#define _CRT_SECURE_NO_WARNINGS

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <process.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <queue>
#include <map>
#include <deque>
using namespace std;



// TODO:  在此处引用程序需要的其他头文件
#include "../Server/common/CommonDef.h"
#include "../Server/common/CommonReq.h"
#include "../Server/include/func.h"
#include "../Server/include/utils.h"
#include "../Server/include/atomic_ops.h"
#include "../Server/include/LockFreeQueue.h"
#include "../Server/iocp.h"