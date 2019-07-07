#include "stdafx.h"

TimeWheel::TimeWheel()
:m_bStop(false), m_uiTick(0)
{
	StartTimer();
}
TimeWheel::~TimeWheel()
{
	m_bStop = true;
}

void TimeWheel::StartTimer()
{
	m_uiTimeWheelNodeID = 0;
	m_lCurrentTime = _getCentiSecond();
	DWORD uiThreadID = 0;
	unsigned int uiID = 0;
	HANDLE handle = (HANDLE)_beginthreadex(NULL, 0, _runTimer, this, 0, NULL);
}

void TimeWheel::UpdateTimer()
{
	while (!m_bStop)
	{
		long curTime = _getCentiSecond();
		long timeVal = (curTime - m_lCurrentTime);
		m_lCurrentTime = curTime;
		for (auto i = 0; i < timeVal; i++)
		{
			ExcuteTimerList();		//先看当前有没有什么定时任务需要处理
			MoveTimeWheelList();	//移动定时任务列表,如果需要移动的话
			ExcuteTimerList();		//处理列表移动之后的定时任务
		}
		Sleep(TIME_INTERVAL);
	}
}

TimeWheelNode* TimeWheel::SetTimer(unsigned int uiMilliSeconds, TimeOutCB TOC, void* pParam, CancelCB CC)
{
	CAutoLock lock(&m_csLock);
	uiMilliSeconds /= TIME_INTERVAL;
	TimeWheelNode* pNode = new TimeWheelNode;
	memset(pNode, 0, sizeof(TimeWheelNode));
	pNode->pParam = pParam;
	pNode->uiExpireTime = uiMilliSeconds + m_uiTick;
	pNode->pTOCB = TOC;
	pNode->pCCB = CC;
	pNode->nTimerID = ++m_uiTimeWheelNodeID;

	m_TimeWheelMap[pNode->nTimerID] = pNode;
	InsertTimeWheelNode(pNode);

	return pNode;
}

void TimeWheel::CancelTimer(unsigned int uiTimerID)
{
	TimeWheelNode* pNode = m_TimeWheelMap[uiTimerID];
	if (NULL == pNode)
	{
		CAutoLock lock(&m_csLock);
		return;
	}
	if (NULL != pNode->pCCB)
	{
		pNode->pCCB(pNode->pParam);
	}
	CAutoLock lock(&m_csLock);
	m_TimeWheelMap.erase(uiTimerID);
	m_TimeWheelList[pNode->uiLevelID][pNode->uiSlotID].pop_back();
	delete pNode;
}

void TimeWheel::ExcuteTimerList()
{
	//首先判断当前时间处于最里面执行圈的哪个槽口
	int nCurrentSlot = (m_uiTick & MASK_INNERMOST);
	//取出对应槽口的定时任务列表，取出后清空
	std::list<TimeWheelNode*> nodeList = m_ExcuteList[nCurrentSlot];
	{
		CAutoLock lock(&m_csLock);
		//nodeList = m_ExcuteList[nCurrentSlot];
		m_ExcuteList[nCurrentSlot].clear();
	}
	//执行当前时间待处理任务列表中待处理的定时任务
	for (auto iter = nodeList.begin(); iter != nodeList.end(); iter++)
	{
		TimeWheelNode* pNode = *iter;
		pNode->pTOCB(pNode->pParam);
		{
			CAutoLock lock(&m_csLock);
			//删除存储在map中的定时器任务结点
			m_TimeWheelMap.erase(pNode->nTimerID);
		}
		delete pNode;
	}
}

void TimeWheel::MoveTimeWheelList()
{
	//m_nTick自增
	unsigned int uiCurrentTick = ++m_uiTick;
	//定义uiMask,uiTime,i临时变量
	unsigned int uiMask = (1 << BIT_INNER);
	unsigned int uiTime = (uiCurrentTick >> BIT_INNER);
	int i = 0;
	//当前时间与上（1111 1111），等于0，即说明当前计时为256的倍数，也就是时间轮转完一圈
	//此时，对应外圈应该转动一格，同时将当前转到的格子中的所有定时任务重新进行分配
	while ((uiCurrentTick & (uiMask - 1)) == 0)
	{
		unsigned int uiMoveSlot = (uiTime & MASK_OUTLEVELS);
		//等于0即代表当前时间不处于当前层级所属外圈，继续循环进行遍历
		if (uiMoveSlot != 0)
		{
			//更新任务
			MoveTimeWheelListEx(m_TimeWheelList[i][uiMoveSlot]);
			m_TimeWheelList[i][uiMoveSlot].clear();
			break;
		}
		uiTime >>= BIT_LEVEL;
		uiMask <<= BIT_LEVEL;
		//层级数自增
		++i;
	}
}

void TimeWheel::MoveTimeWheelListEx(std::list<TimeWheelNode*>& nodeList)
{
	CAutoLock lock(&m_csLock);
	//遍历该槽上的所有定时任务，重新插入到时间轮中
	for (auto iter = nodeList.begin(); iter != nodeList.end();)
	{
		TimeWheelNode* pNode = *iter;
		InsertTimeWheelNode(pNode);
		iter = nodeList.erase(iter++);
	}
}

void TimeWheel::InsertTimeWheelNode(TimeWheelNode* pNode)
{
	//当前node剩余到期时间，单位ms
	unsigned int uiRemainTime = pNode->uiExpireTime - m_uiTick;
	//当前node剩余到期时间在256以内，即已经到快要执行的时间了，放入最里面待执行圈对应槽口
	if ((uiRemainTime | MASK_INNERMOST) == MASK_INNERMOST)
	{
		//计算node需要插入的位置
		unsigned int uiInsertSlot = (uiRemainTime & MASK_INNERMOST);
		pNode->bToExcute = true;
		pNode->uiSlotID = uiInsertSlot;
		m_ExcuteList[uiInsertSlot].push_back(pNode);
	}
	//不是即将执行的，重新分配存放位置
	else
	{
		unsigned int uiMask = ((1 << BIT_INNER) << BIT_LEVEL);
		int i = 0;
		for (i = 0; i < TIME_WHEEL_LEVEL_SLOT; i++)
		{
			//当前node剩余过期时间如果属于该层级时，取得该层级对应掩码，否则继续循环
			if ((uiRemainTime | (uiMask - 1)) == (uiMask - 1))
			{
				break;
			}
			uiMask <<= BIT_LEVEL;
		}
		//计算当前node在当前层级所属槽口
		unsigned int uiInsertSlot = ((pNode->uiExpireTime >> (BIT_INNER + i * BIT_LEVEL)) & (uiMask - 1));
		pNode->bToExcute = false;
		pNode->uiLevelID = i;
		pNode->uiSlotID = uiInsertSlot;
		m_TimeWheelList[i][uiInsertSlot].push_back(pNode);
	}
}

unsigned TimeWheel::_runTimer(void* pParam)
{
	TimeWheel* pTimeWheel = (TimeWheel*)pParam;
	pTimeWheel->UpdateTimer();
	return 0;
}