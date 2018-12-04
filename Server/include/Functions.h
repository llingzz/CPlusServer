#pragma once

#include <Windows.h>
#include <time.h>
#include <sys/timeb.h>

#define TIME_ACCURACY	20		//时间精度

#define DELTA_EPOCH_IN_MICROSECS	11644473600000000Ui64
static VOID(WINAPI *fnGetSystemTimePreciseAsFileTime)(LPFILETIME) = NULL;
struct timezone {
	int  tz_minuteswest; /* minutes W of Greenwich */
	int  tz_dsttime;     /* type of dst correction */
};

static void _gettimeofday(struct timeval* tv)
{
	union{
		long long llNs;
		FILETIME ft;
	}now;

	GetSystemTimeAsFileTime(&now.ft);
	tv->tv_usec = (long)((now.llNs / 10LL) % 1000000LL);
	tv->tv_sec = (long)((now.llNs - 116444736000000000LL) / 10000000LL);
}

static void _gettimeofday_1(struct timeval* tv, struct timezone* tz)
{
	FILETIME ft;
	unsigned __int64 tmpres = 0;
	static int tzflag;

	if (NULL == fnGetSystemTimePreciseAsFileTime) {
		//_initHighResAbsoluteTime();
		FARPROC fp;
		HMODULE module;
		if (fnGetSystemTimePreciseAsFileTime != NULL)
			return;
		fnGetSystemTimePreciseAsFileTime = GetSystemTimeAsFileTime;
		module = GetModuleHandleA("kernel32.dll");
		if (module) {
			fp = GetProcAddress(module, "GetSystemTimePreciseAsFileTime");
			if (fp) {
				fnGetSystemTimePreciseAsFileTime = (VOID(WINAPI*)(LPFILETIME)) fp;
			}
		}
	}
	if (NULL != tv) {
		fnGetSystemTimePreciseAsFileTime(&ft);

		tmpres |= ft.dwHighDateTime;
		tmpres <<= 32;
		tmpres |= ft.dwLowDateTime;
		/*converting file time to unix epoch*/
		tmpres /= 10;  /*convert into microseconds*/
		tmpres -= DELTA_EPOCH_IN_MICROSECS;
		tv->tv_sec = (long)(tmpres / 1000000UL);
		tv->tv_usec = (long)(tmpres % 1000000UL);
	}
	if (NULL != tz) {
		if (!tzflag) {
			_tzset();
			tzflag++;
		}
		tz->tz_minuteswest = _timezone / 60;
		tz->tz_dsttime = _daylight;
	}
}

//返回当前毫秒数（精度单位：20ms）
static long _getCentiSecond()
{
	timeval tv;
	_gettimeofday(&tv/*, NULL*/);

	return (tv.tv_sec * 1000 / TIME_ACCURACY + tv.tv_usec / (1000 * TIME_ACCURACY));
}
//返回当前秒数（精度单位：20ms）
static long _getSencond()
{
	timeval tv;
	_gettimeofday(&tv);

	return (tv.tv_sec * 1000 * 1000 / TIME_ACCURACY + tv.tv_usec / (1000 * 1000 * TIME_ACCURACY));
}

//Character conversions 字符转换
static void StringToTchar(std::string str, TCHAR* szTchar)
{
#ifdef UNICODE
	_stprintf_s(szTchar, MAX_PATH, _T("%S"), str.c_str());
#else
	_stprintf_s(szTchar, MAX_PATH, _T("%s"), str.c_str());
#endif // UNICODE
}
static void TcharToString(TCHAR* szTchar, std::string& str)
{
	int nLen = WideCharToMultiByte(CP_ACP, 0, szTchar, -1, NULL, 0, NULL, NULL);
	char* pTemp = new char[nLen*sizeof(char)];
	WideCharToMultiByte(CP_ACP, 0, szTchar, -1, pTemp, nLen, NULL, NULL);
	std::string strTemp(pTemp);
	str = strTemp;
	delete[] pTemp;
}
static void StringToChar(std::string str, char* szChar)
{
	strcpy_s(szChar, str.length() + 1, str.c_str());
}
static void CharToString(char* szChar, std::string& str)
{
	std::string strTemp(szChar);
	str = strTemp;
}
static void TcharToChar(const TCHAR* tar, char* car)
{
	int nLen = WideCharToMultiByte(CP_ACP, 0, tar, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, tar, -1, car, nLen, NULL, NULL);
}
static void CharToTchar(const char* car, TCHAR* tar, const int nCharSize)
{
	int nLen = MultiByteToWideChar(CP_ACP, 0, car, nCharSize + 1, NULL, 0);
	MultiByteToWideChar(CP_ACP, 0, car, nCharSize + 1, tar, nLen/**sizeof(TCHAR)*/);
}