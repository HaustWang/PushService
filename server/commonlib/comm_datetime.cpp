#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "comm_datetime.h"

int32_t get_string_datetime(const time_t time, char* pszBuffer)
{
	if (NULL == pszBuffer)
	{
		return -1;
	}

	struct tm stTime;
	struct tm* pstTime = NULL;
	pstTime = localtime_r(&time, &stTime);
	if (NULL == pstTime)
	{
		return -1;
	}

	sprintf(pszBuffer, "%04d-%02d-%02d %02d:%02d:%02d",
		stTime.tm_year + 1900, stTime.tm_mon + 1, stTime.tm_mday,
		stTime.tm_hour, stTime.tm_min, stTime.tm_sec);

	return 0;
}

int32_t get_struct_datetime(const time_t time, stDateTime& out)
{
	struct tm *pstTime = NULL;
	struct tm stTime;
	time_t tTmp = time;

	
	pstTime = localtime_r(&tTmp, &stTime);
	if(NULL == pstTime)
	{
		return -1;
	}

	out.m_iYear = stTime.tm_year + 1900;
	out.m_iMon = stTime.tm_mon + 1;
	out.m_iDay = stTime.tm_mday;
	out.m_iHour = stTime.tm_hour;
	out.m_iMin = stTime.tm_min;
	out.m_iSec = stTime.tm_sec;
	out.m_iMSec = 0;

	return 0;
}

int32_t get_current_string_datetime(char* pszBuffer)
{
	time_t now;
	time(&now);

	return get_string_datetime((const time_t) now, pszBuffer);
}

int32_t get_current_string_datetime_useconds(char* pszBuffer)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    struct tm *pstTime = NULL;
    struct tm stTime;
    time_t tTmp = tv.tv_sec;


    pstTime = localtime_r(&tTmp, &stTime);
    if(NULL == pstTime)
    {
        return -1;
    }

    sprintf(pszBuffer, "%04d-%02d-%02d %02d:%02d:%02d.%06ld",
        stTime.tm_year + 1900, stTime.tm_mon + 1, stTime.tm_mday,
        stTime.tm_hour, stTime.tm_min, stTime.tm_sec,
        tv.tv_usec);

    return 0;
}

int32_t get_current_struct_datetime(stDateTime& out)
{
	time_t now;
	time(&now);
	return get_struct_datetime((const time_t)now, out);
}

int32_t get_current_string_date(char* strDate)
{
	struct tm *pstTm = NULL;
	time_t now  =0;

	if(NULL == strDate)
	{
		return -1;
	}

	time(&now);
	strDate[0] = '\0';

	pstTm = localtime(&now);
	if(NULL == pstTm)
	{
		return -1;
	}

	sprintf(strDate, "%04d-%02d-%02d", pstTm->tm_year + 1900, pstTm->tm_mon + 1, pstTm->tm_mday);
	return 0;
}

int32_t get_current_string_time(char* strTime)
{
	struct tm *pstTm = NULL;
	time_t now = 0;

	if(NULL == strTime)
	{
		return -1;
	}

	time(&now);
	strTime[0] = '\0';

	pstTm = localtime(&now);
	if(NULL == pstTm)
	{
		return -1;
	}

	sprintf(strTime, "%02d:%02d:%02d", pstTm->tm_hour, pstTm->tm_min, pstTm->tm_sec);
	return 0;
}

int32_t get_current_string_bill(char* strBill)
{
	struct tm *pstTm = NULL;
	time_t now  =0;

	if(NULL == strBill)
	{
		return -1;
	}

	time(&now);
	strBill[0] = '\0';

	pstTm = localtime(&now);
	if(NULL == pstTm)
	{
		return -1;
	}

	sprintf(strBill, "%04d-%02d-%02d-%02d", pstTm->tm_year + 1900, pstTm->tm_mon + 1, pstTm->tm_mday, pstTm->tm_hour);
	return 0;
}

int32_t get_struct_datetime_from_string(const char* strDateTime, stDateTime& out)
{
	char *pTime = NULL;

	if (NULL == strDateTime) 
	{
		return -1;
	}

	pTime = (char*)&strDateTime[0];

	//年
	pTime[4] = '\0';
	out.m_iYear = atoi(pTime);
	pTime += 5;
	
	//月
	pTime[2] = '\0';
	out.m_iMon= atoi(pTime);
	pTime += 3;

	//日
	pTime[2] = '\0';
	out.m_iDay= atoi(pTime);
	pTime += 3;

	//小时
	pTime[2] = '\0';
	out.m_iHour= atoi(pTime);
	pTime += 3;

	//分钟
	pTime[2] = '\0';
	out.m_iMin= atoi(pTime);
	pTime += 3;

	//秒
	pTime[2] = '\0';
	out.m_iSec= atoi(pTime);
	pTime += 3;	

	return 0;
}


int32_t get_string_from_struct_datetime(const stDateTime& stDate,char* pszBuffer)
{
	if (NULL == pszBuffer)
	{
		return  -1;
	}
	sprintf(pszBuffer, "%04d-%02d-%02d %02d:%02d:%02d",
		stDate.m_iYear , stDate.m_iMon, stDate.m_iDay,
		stDate.m_iHour, stDate.m_iMin, stDate.m_iSec);
	return  0;
}
void plus_timevalue(struct timeval& src, const struct timeval& plus)
{
	timeval tvTemp;
	tvTemp.tv_sec = src.tv_sec + plus.tv_sec;
	tvTemp.tv_sec += ((src.tv_usec+plus.tv_usec)/usec_of_one_second);
	tvTemp.tv_usec = ((src.tv_usec+plus.tv_usec)%usec_of_one_second);

	src.tv_sec = tvTemp.tv_sec;
	src.tv_usec = tvTemp.tv_usec;
}

void minus_timevalue(struct timeval& src, const struct timeval& minus)
{
	timeval tvTemp;

	if( src.tv_usec < minus.tv_usec )
	{
		tvTemp.tv_usec = (usec_of_one_second + src.tv_usec) - minus.tv_usec;
		tvTemp.tv_sec = src.tv_sec - minus.tv_sec - 1;
	}
	else
	{
		tvTemp.tv_usec = src.tv_usec - minus.tv_usec;
		tvTemp.tv_sec  = src.tv_sec - minus.tv_sec;
	}

	src.tv_sec = tvTemp.tv_sec;
	src.tv_usec = tvTemp.tv_usec;
}

//把类型为"YYYY-MM-DD HH:MM:SS"格式的datetime转换为结构时间struct tm
//成功返回 0, 否则其它
int32_t get_struct_tm_from_datetime_string( struct tm& struct_Tm, const char* pszDateTime )
{
	if (NULL == pszDateTime)
	{
		return -1;
	}

	char* pszTmp = NULL;
	char szTimeBuf[32];

	szTimeBuf[0] = '\0';
	memset(&struct_Tm, 0, sizeof(struct_Tm));

	strncpy(szTimeBuf, pszDateTime, sizeof(szTimeBuf)-1);

	if (strlen(pszDateTime) < 19) //"YYYY-MM-DD HH:MM:SS"格式至少有19个字符
	{
		return -1;
	}

	pszTmp = &szTimeBuf[0];
	//取出年
	pszTmp[4] = '\0';
	struct_Tm.tm_year = atoi(pszTmp) - 1900;
	pszTmp += 5;

	pszTmp[2] = '\0';
	struct_Tm.tm_mon = atoi(pszTmp) -1;
	pszTmp += 3;

	pszTmp[2] = '\0';
	struct_Tm.tm_mday = atoi(pszTmp);
	pszTmp += 3;

	pszTmp[2] = '\0';
	struct_Tm.tm_hour = atoi(pszTmp);
	pszTmp += 3;

	pszTmp[2] = '\0';
	struct_Tm.tm_min = atoi(pszTmp);
	pszTmp += 3;

	pszTmp[2] = '\0';
	struct_Tm.tm_sec = atoi(pszTmp);
	pszTmp += 3;	


	return 0;

}

int32_t get_time_t_from_datetime_string( time_t& tmTime, const char* pszDateTime )
{
	if (NULL == pszDateTime)
	{
		return -1;
	}

	struct tm stTime;
	
	if (0 != get_struct_tm_from_datetime_string(stTime, pszDateTime))
	{
		return -1;
	}

	time_t tTime = 0;
	tTime = mktime(&stTime);
	if ((time_t)(-1) == tTime)
	{
		return -1;
	}

	tmTime = tTime;

	return 0;
}

int32_t get_string_date_from_time_t( const time_t time, char* date )
{
	struct stDateTime tmpDateTime;
	if (0 != get_struct_datetime(time, tmpDateTime))
	{
		return -1;
	}

	sprintf(date, "%04d-%02d-%02d", tmpDateTime.m_iYear, tmpDateTime.m_iMon, tmpDateTime.m_iDay);
	
	return 0;
}

int32_t get_string_now_date( char* date )
{
	time_t now = time(NULL);

	return get_string_date_from_time_t((const time_t)now, date);
}
int64_t GetTimeUs()
{
    struct timeval t_start;
    gettimeofday(&t_start, NULL);
    return t_start.tv_sec*1000000 + t_start.tv_usec;
}
int64_t GetTimeMs()
{
    struct timeval t_start;
    gettimeofday(&t_start, NULL);
    return t_start.tv_sec*1000 + (t_start.tv_usec/1000);
}
bool is_today(time_t tmTime)                             
{                                                        
    stDateTime stTime;
    if (0 == get_struct_datetime(tmTime,  stTime)) 
    {                                                    
        stDateTime stCurTime;
        get_current_struct_datetime(stCurTime);          
        if (stTime.m_iYear == stCurTime.m_iYear &&       
                stTime.m_iMon  == stCurTime.m_iMon  &&
                stTime.m_iDay  == stCurTime.m_iDay)          
        {                                                
            return true;
        }
    }
    return false;
}
bool is_yesterday(time_t tmTime)
{
    struct tm stTime;
    struct tm stCurTime;
    localtime_r(&tmTime, &stTime);
    time_t now_time = time(NULL);
    localtime_r(&now_time, &stCurTime);
    if(stTime.tm_year == stCurTime.tm_year &&
            stTime.tm_yday+1 == stTime.tm_yday)
    {
        return true;
    }
    else if(stTime.tm_year+1 == stCurTime.tm_year &&
            stTime.tm_mon == 11 && stCurTime.tm_mon == 0 &&
            stTime.tm_mday == 31 && stCurTime.tm_mday == 1)
    {
        //12月31号，和1月1号
        return true;
    }
    return false;
}

bool is_same_day(time_t t1, time_t t2)
{                                                 
    struct tm t1_tm;                              
    localtime_r(&t1, &t1_tm);                     

    struct tm t2_tm;                              
    localtime_r(&t2, &t2_tm);                     

    if(t1_tm.tm_yday == t2_tm.tm_yday &&          
            t1_tm.tm_year == t2_tm.tm_year)       
    {                                             
        return true;
    }                                             
    return false;
}

bool is_same_hour(time_t t1, time_t t2)
{
    struct tm st_tm1;
    struct tm st_tm2;
    localtime_r(&t1, &st_tm1);
    localtime_r(&t2, &st_tm2);
    if(st_tm1.tm_year == st_tm2.tm_year &&
            st_tm1.tm_yday == st_tm2.tm_yday &&
            st_tm1.tm_hour == st_tm2.tm_hour)
    {
        return true;
    }
    return false;
}


bool IsSameMonth(time_t t1, time_t t2)
{
    struct tm st_tm1;
    struct tm st_tm2;
    localtime_r(&t1, &st_tm1);
    localtime_r(&t2, &st_tm2);

    if (st_tm1.tm_year == st_tm2.tm_year &&
            st_tm1.tm_mon == st_tm2.tm_mon)
    {
        return true;
    }

    return false;
}

int64_t timersub_ms(const struct timeval *late_t, const struct timeval *early_t)     
{                                                                                
    struct timeval delta;                                                        
    timersub(late_t, early_t, &delta);
    return delta.tv_sec * 1000 + delta.tv_usec / 1000;                           
}   

int64_t timersub_us(const struct timeval *late_t, const struct timeval *early_t) 
{   
    struct timeval delta;                                                        
    timersub(late_t, early_t, &delta);
    return (int64_t)delta.tv_sec * 1000 * 1000 + delta.tv_usec;
}   

int32_t get_current_day_passed_second(time_t t)
{
	struct tm *pstTime = NULL;
	struct tm stTime;

	pstTime = localtime_r(&t, &stTime);
	if (NULL == pstTime)
	{
		return 0;
	}

	int iTimeSec = stTime.tm_hour * 3600 + stTime.tm_min * 60 + stTime.tm_sec;
	return iTimeSec;
}

time_t GetNextHourOfTime(int hour)
{
    hour = hour % 24;

    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);

    struct tm tm_today_zero = tm_now;
    tm_today_zero.tm_sec = 0;
    tm_today_zero.tm_min = 0;
    tm_today_zero.tm_hour = 0;
    time_t today_zero = mktime(&tm_today_zero);

    //今天这个小时还没过
    if (tm_now.tm_hour < hour)
    {
        return today_zero + 3600 * hour;
    }
    //需要跨天了
    else
    {
        return today_zero + 3600 * (24 + hour);
    }
}

time_t GetNextDayZeroTime(time_t now_time)
{
    struct tm tm_today_zero;
    localtime_r(&now_time, &tm_today_zero);
    tm_today_zero.tm_sec = 0;
    tm_today_zero.tm_min = 0;
    tm_today_zero.tm_hour = 0;
    time_t today_zero = mktime(&tm_today_zero);
    return today_zero + (24*60*60);
}

 int opt_gettimeofday(struct timeval *tv, void *not_used)
{
    static volatile uint64_t walltick;
    static volatile struct timeval walltime;
    static volatile long lock = 0;
    const unsigned int max_ticks = CPU_SPEED_GB*1000*REGET_TIME_US;

    if(walltime.tv_sec==0 || (RDTSC()-walltick) > max_ticks)
    {
        if(CAS(&lock, 0UL, 1UL)) // try lock
        {
            gettimeofday((struct timeval*)&walltime, (struct timezone*)not_used);
            walltick = RDTSC();
            lock = 0; // unlock
        }
        else // if try lock failed, use system time
        {
            return gettimeofday(tv, (struct timezone*)not_used);
        }
    }
    memcpy(tv, (void*)&walltime, sizeof(struct timeval));
    return 0;
}

 time_t opt_time(time_t *t) 
{
    struct timeval tv;
    opt_gettimeofday(&tv, NULL);
    if(t) *t = tv.tv_sec;
    return tv.tv_sec;
}

