#ifndef __COMM_DATETIME_H__
#define __COMM_DATETIME_H__

#include <sys/time.h>
#include <utime.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __x86_64__
#define RDTSC() ({ register uint32_t a,d; __asm__ __volatile__( "rdtsc" : "=a"(a), "=d"(d)); (((uint64_t)a)+(((uint64_t)d)<<32)); })
#else
#define RDTSC() ({ register uint64_t tim; __asm__ __volatile__( "rdtsc" : "=A"(tim)); tim; })
#endif

// atomic operations
#ifdef __x86_64__
    #define ASUFFIX "q"
#else
    #define ASUFFIX "l"
#endif
#define CAS(ptr, val_old, val_new)({ char ret; __asm__ __volatile__("lock; cmpxchg" ASUFFIX " %2,%0; setz %1": "+m"(*ptr), "=q"(ret): "r"(val_new),"a"(val_old): "memory"); ret;})

#define REGET_TIME_US   1000
#define CPU_SPEED_GB    2  // assume a 2GB CPU

    enum
    {
        usec_of_one_second	= 1000000,	//
    };

    struct stDateTime
    {
        int32_t m_iYear;
        int32_t m_iMon;
        int32_t m_iDay;
        int32_t m_iHour;
        int32_t m_iMin;
        int32_t m_iSec;
        int32_t m_iMSec;
    };

    int32_t get_string_datetime(const time_t time, char* pszBuffer);
    int32_t get_struct_datetime(const time_t time, stDateTime& out);

    int32_t get_current_string_datetime(char* pszBuffer);
    int32_t get_current_string_datetime_useconds(char* pszBuffer);
    int32_t get_current_struct_datetime(stDateTime& out);

    int32_t get_current_string_date(char* strDate);
    int32_t get_current_string_time(char* strTime);
    int32_t get_current_string_bill(char* strBill);

    int32_t get_struct_datetime_from_string(const char* strDateTime, stDateTime& out);
    int32_t get_string_from_struct_datetime(const stDateTime& stDate,char* pszBuffer);

    void plus_timevalue(struct timeval& src, const struct timeval& plus);
    void minus_timevalue(struct timeval& src, const struct timeval& minus);

    int32_t get_struct_tm_from_datetime_string(struct tm& struct_Tm, const char* pszDateTime);
    int32_t get_time_t_from_datetime_string(time_t& tmTime, const char* pszDateTime);

    //由调用者保证传入的date缓冲区的合法性
    int32_t get_string_date_from_time_t(const time_t time, char* date);
    int32_t get_string_now_date(char* date);

    int64_t GetTimeUs();
    int64_t GetTimeMs();

    bool is_today(time_t tmTime);
    bool is_yesterday(time_t tmTime);

    //判断是否是同一天
    bool is_same_day(time_t t1, time_t t2);
    bool is_same_hour(time_t t1, time_t t2);
    bool IsSameMonth(time_t t1, time_t t2);

    int64_t timersub_us(const struct timeval *late_t, const struct timeval *early_t);
    int64_t timersub_ms(const struct timeval *late_t, const struct timeval *early_t);
    int32_t get_current_day_passed_second(time_t t);

    //得到最近的下一个指定hour的时间
    time_t GetNextHourOfTime(int hour);

    time_t GetNextDayZeroTime(time_t now_time);

 int opt_gettimeofday(struct timeval *tv, void *not_used);
 time_t opt_time(time_t *t);

#ifdef __cplusplus
}
#endif

#endif //__COMMON_DATETIME_H__
