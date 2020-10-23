#pragma once

#include <windows.h>
#include <string.h>
#include <fstream>

#include "lock.h"

using namespace std;

/*单例模板*/
template<typename T>
class CSingle {

protected:
	CSingle(void) {}
	~CSingle(void) {}

private:
	CSingle(const CSingle& singleton);

public:
	static T* GetInstance()
	{
		static T instance;
		return &instance;
	}
};
// 日志级别枚举
enum enLogLevel {
	enDEFAULT = 0,
	enINFO,
	enDEBUG,
	enWARN,
	enTRACE,
	enERROR,
	enFATAL,
};
// 自定义日志记录类
class CLog : public CSingle<CLog> {
public:
	CLog()
	{
		m_nLogLevel = enDEFAULT;
		m_nCurrentIndex = 0;
		m_pFp = NULL;
		ZeroMemory(m_szFilePath, MAX_PATH);
		ZeroMemory(m_szFileName, MAX_PATH);
		ZeroMemory(m_szLogRrefix, 8192);
		ZeroMemory(m_szLogContent, 8192);
	}
	~CLog()
	{
		m_nLogLevel = enDEFAULT;
		if (m_pFp) {
			fclose(m_pFp);
			m_pFp = NULL;
		}
		ZeroMemory(m_szFilePath, MAX_PATH);
		ZeroMemory(m_szFileName, MAX_PATH);
		ZeroMemory(m_szLogRrefix, 8192);
		ZeroMemory(m_szLogContent, 8192);
	}

public:
	void WriteLogFile(const char* fmt, ...)
	{
		CAutoLock lock(&m_csLock);
		GetLocalTime(&m_sysTime);
		GetCurrentTimeRrefix();
		GetCurrentLevel(m_nLogLevel);
		GetFilePathAndName();

		va_list ap;
		va_start(ap, fmt);
		vsprintf_s(m_szLogContent, fmt, ap);
		va_end(ap);

		string szRes = m_szLogRrefix;
		szRes.append(m_szLogContent);
		strcpy_s(m_szLogContent, szRes.length() + 1, szRes.c_str());

		if (NULL == m_pFp)
		{
			fopen_s(&m_pFp, m_szFilePath, "a+");
			while (NULL == m_pFp)
			{
				fopen_s(&m_pFp, m_szFilePath, "a+");//确保文件能被打开，日志能被写入
			}
		}
		fwrite(m_szLogContent, strlen(m_szLogContent), 1, m_pFp);
		fwrite("\n", 1, 1, m_pFp);
		fflush(m_pFp);
		auto nFileSize = ftell(m_pFp);

		fclose(m_pFp);
		m_pFp = NULL;
	}
	void WriteLogConsole(const char* fmt, ...)
	{
		CAutoLock lock(&m_csLock);
		GetLocalTime(&m_sysTime);
		GetCurrentTimeRrefix();
		GetCurrentLevel(m_nLogLevel);

		va_list ap;
		va_start(ap, fmt);
		vsprintf_s(m_szLogContent, fmt, ap);
		va_end(ap);

		string szRes = m_szLogRrefix;
		szRes.append(m_szLogContent);
		strcpy_s(m_szLogContent, szRes.length() + 1, szRes.c_str());
		printf_s("%s\n", m_szLogContent);
	}
	void WriteLogFileEx(const char* fmt, ...)
	{
		CAutoLock lock(&m_csLock);
		GetLocalTime(&m_sysTime);
		GetCurrentTimeRrefix();
		GetCurrentLevel(m_nLogLevel);
		GetFilePathAndName();

		va_list ap;
		va_start(ap, fmt);
		vsprintf_s(m_szLogContent, fmt, ap);
		va_end(ap);

		string szRes = m_szLogRrefix;
		szRes.append(m_szLogContent);
		strcpy_s(m_szLogContent, szRes.length() + 1, szRes.c_str());

		ofstream log;
		log.open(m_szFilePath, ios::out | ios::app);
		while (!log.is_open())
		{
			log.open(m_szFilePath, ios::out | ios::app);
		}
		log.write(m_szLogContent, strlen(m_szLogContent));
		log.write("\n", 1);
		log.close();
	}
	CLog* SetLogLevel(int nLevel)
	{
		SetConsoleColor((enLogLevel)nLevel);
		m_nLogLevel = nLevel;
		return this;
	}

private:
	void GetCurrentTimeRrefix()
	{
		sprintf_s(m_szLogRrefix, "[%04d-%02d-%02d %02d:%02d:%02d:%3d]", m_sysTime.wYear, m_sysTime.wMonth, m_sysTime.wDay, m_sysTime.wHour, m_sysTime.wMinute, m_sysTime.wSecond, m_sysTime.wMilliseconds);
	}
	void GetCurrentLevel(int nLevel)
	{
		char szLoglevel[8];
		switch (nLevel)
		{
		case enDEBUG:
			strcpy_s(szLoglevel, "DEBUG");
			break;
		case enWARN:
			strcpy_s(szLoglevel, "WARNS");
			break;
		case enTRACE:
			strcpy_s(szLoglevel, "TRACE");
			break;
		case enERROR:
			strcpy_s(szLoglevel, "ERROR");
			break;
		case enFATAL:
			strcpy_s(szLoglevel, "FATAL");
			break;
		case enINFO:
		default:
			strcpy_s(szLoglevel, "INFOS");
			break;
		}
		sprintf_s(m_szLogRrefix, "%s[%5d][%s]:", m_szLogRrefix, ::GetCurrentThreadId(), szLoglevel);
	}
	void GetFilePathAndName()
	{
		sprintf_s(m_szFileName, "\\%s%04d%02d%02d.log", "CPlusServer", m_sysTime.wYear, m_sysTime.wMonth, m_sysTime.wDay);

		TCHAR szFilePath[MAX_PATH];
		GetModuleFileName(GetModuleHandle(NULL), szFilePath, MAX_PATH - 1);
		int nLen = WideCharToMultiByte(CP_ACP, 0, (LPCWCH)szFilePath, -1, NULL, 0, NULL, NULL);
		WideCharToMultiByte(CP_ACP, 0, (LPCWCH)szFilePath, -1, m_szFilePath, nLen, NULL, NULL);
		const char* ptr = strrchr(m_szFilePath, '\\');

		string strStr = m_szFilePath;
		auto nIndex = strStr.find(ptr);
		string strRes = strStr.substr(0, nIndex);
		strRes.append(m_szFileName);

		strcpy_s(m_szFilePath, strRes.length() + 1, strRes.c_str());
	}
	bool SetConsoleColor(enLogLevel enLevel)
	{
		WORD wAttributes = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN;
		switch (enLevel)
		{
		case enINFO:
		case enTRACE:
		case enDEBUG:
			wAttributes = FOREGROUND_GREEN | FOREGROUND_GREEN | FOREGROUND_GREEN;
			break;
		case enWARN:
			wAttributes = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN;
			break;
		case enERROR:
		case enFATAL:
			wAttributes = FOREGROUND_RED | FOREGROUND_RED | FOREGROUND_RED;
			break;
		case enDEFAULT:
		default:
			wAttributes = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN;
			break;
		}
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hConsole == INVALID_HANDLE_VALUE)
		{
			return FALSE;
		}
		return SetConsoleTextAttribute(hConsole, wAttributes);
	}

private:
	int			m_nLogLevel;
	int			m_nCurrentIndex;
	FILE*		m_pFp;
	CCritSec	m_csLock;
	char		m_szFilePath[MAX_PATH];
	char		m_szFileName[MAX_PATH];
	char		m_szLogRrefix[8192];
	char		m_szLogContent[8192];
	SYSTEMTIME	m_sysTime;
};

// 日志宏定义
#if _DEBUG
// 文件日志快捷宏
#define		myLogFileI(fmt, ...) CLog::GetInstance()->SetLogLevel(enINFO)->WriteLogFile(fmt, __VA_ARGS__)
#define		myLogFileD(fmt, ...) CLog::GetInstance()->SetLogLevel(enDEBUG)->WriteLogFile(fmt, __VA_ARGS__)
#define		myLogFileW(fmt, ...) CLog::GetInstance()->SetLogLevel(enWARN)->WriteLogFile(fmt, __VA_ARGS__)
#define		myLogFileT(fmt, ...) CLog::GetInstance()->SetLogLevel(enTRACE)->WriteLogFile(fmt, __VA_ARGS__)
#define		myLogFileE(fmt, ...) CLog::GetInstance()->SetLogLevel(enERROR)->WriteLogFile(fmt, __VA_ARGS__)
#define		myLogFileF(fmt, ...) CLog::GetInstance()->SetLogLevel(enFATAL)->WriteLogFile(fmt, __VA_ARGS__)
#else
//文件日志快捷宏
#define		myLogFileI(fmt, ...) CLog::GetInstance()->SetLogLevel(enINFO)->WriteLogFileEx(fmt, __VA_ARGS__)
#define		myLogFileD(fmt, ...) CLog::GetInstance()->SetLogLevel(enDEBUG)->WriteLogFileEx(fmt, __VA_ARGS__)
#define		myLogFileW(fmt, ...) CLog::GetInstance()->SetLogLevel(enWARN)->WriteLogFileEx(fmt, __VA_ARGS__)
#define		myLogFileT(fmt, ...) CLog::GetInstance()->SetLogLevel(enTRACE)->WriteLogFileEx(fmt, __VA_ARGS__)
#define		myLogFileE(fmt, ...) CLog::GetInstance()->SetLogLevel(enERROR)->WriteLogFileEx(fmt, __VA_ARGS__)
#define		myLogFileF(fmt, ...) CLog::GetInstance()->SetLogLevel(enFATAL)->WriteLogFileEx(fmt, __VA_ARGS__)
#endif
// 控制台打印快捷宏
#define		myLogConsoleI(fmt, ...) CLog::GetInstance()->SetLogLevel(enINFO)->WriteLogConsole(fmt, __VA_ARGS__)
#define		myLogConsoleD(fmt, ...) CLog::GetInstance()->SetLogLevel(enDEBUG)->WriteLogConsole(fmt, __VA_ARGS__)
#define		myLogConsoleW(fmt, ...) CLog::GetInstance()->SetLogLevel(enWARN)->WriteLogConsole(fmt, __VA_ARGS__)
#define		myLogConsoleT(fmt, ...) CLog::GetInstance()->SetLogLevel(enTRACE)->WriteLogConsole(fmt, __VA_ARGS__)
#define		myLogConsoleE(fmt, ...) CLog::GetInstance()->SetLogLevel(enERROR)->WriteLogConsole(fmt, __VA_ARGS__)
#define		myLogConsoleF(fmt, ...) CLog::GetInstance()->SetLogLevel(enFATAL)->WriteLogConsole(fmt, __VA_ARGS__)
