#pragma once

#include <Windows.h>
#include <time.h>
#include <sys/timeb.h>

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

static long _getCentiSecond()
{
	timeval tv;
	_gettimeofday(&tv);

	return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}