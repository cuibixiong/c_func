/**************************************************************************
File name	:time_interface.h
Description	:API to get and set time or alarm
Company		:Sunnorth
Date		:Oct 28,2008
Version		:
Modify log	:
****************************************************************************/

#ifndef _TIME_INTERFACE_H_
#define	_TIME_INTERFACE_H_

typedef struct Time_Inter_s{
	unsigned int tm_sec;
	unsigned int tm_min;
	unsigned int tm_hour;
	unsigned int tm_day;
	unsigned int tm_month;
	unsigned int tm_year;
}Time_Inter_t,*pTime_Inter_t;

int GetSysTime(Time_Inter_t* pstTime);

int GetRTCTime(Time_Inter_t* pstTime);

int SetSysTime(Time_Inter_t* pstTime);

int SetSleepTime(Time_Inter_t* pstTime);

int NetTimerSync(int iCityNum);

#endif //_TIME_INTERFACE_H_
