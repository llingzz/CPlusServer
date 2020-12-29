#pragma once
#include <tchar.h>
#include <Windows.h>

static std::string TCHAR2CHAR(TCHAR* pT)
{
	int nLen = WideCharToMultiByte(CP_ACP, 0, (LPCWCH)pT, -1, NULL, 0, NULL, NULL);
	char* szFilePath = new char[nLen];
	WideCharToMultiByte(CP_ACP, 0, (LPCWCH)pT, -1, szFilePath, nLen, NULL, NULL);
	std::string strRes = std::string(szFilePath);
	delete[] szFilePath;
	return strRes;
}

static void CHAR2TCHAR(TCHAR* pT, const char* p)
{
	int wnLen = MultiByteToWideChar(CP_ACP, NULL, p, strlen(p), NULL, 0);
	MultiByteToWideChar(CP_ACP, 0, p, strlen(p) + 1, (LPWSTR)pT, wnLen + 1);
}
