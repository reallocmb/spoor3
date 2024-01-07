#include"spoor_internal.h"

#include<time.h>

struct tm spoor_time_convert_to_struct_tm(SpoorTime *spoor_time)
{
    time_t time_sec = time(NULL);
    struct tm *tm_time = localtime(&time_sec);
    tm_time->tm_mday = spoor_time->day;
    tm_time->tm_mon = spoor_time->mon;
    tm_time->tm_year = spoor_time->year;
    tm_time->tm_hour = spoor_time->hour;
    tm_time->tm_min = spoor_time->min;

    return *tm_time;
}

SpoorTime spoor_time_struct_tm_convert(time_t *time)
{
   return *(SpoorTime *)localtime(time);
}

int32_t spoor_time_compare_day(SpoorTime *spoor_time0, SpoorTime *spoor_time1)
{
    return spoor_time0->day - spoor_time1->day + spoor_time0->mon - spoor_time1->mon + spoor_time0->year - spoor_time1->year;
}

void spoor_time_minutes_add(SpoorTime *spoor_time, int32_t minutes)
{
    struct tm tm_time = spoor_time_convert_to_struct_tm(spoor_time);
    time_t time_sec = mktime(&tm_time);
    time_sec += minutes * 60;
    struct tm *tmp = localtime(&time_sec);
    *spoor_time = *(SpoorTime *)tmp;
}

void spoor_time_days_add(SpoorTime *spoor_time, int32_t days)
{
    struct tm tm_time = spoor_time_convert_to_struct_tm(spoor_time);
    time_t time_sec = mktime(&tm_time);
    time_sec += days * 60 * 60 * 24;
    struct tm *tmp = localtime(&time_sec);
    *spoor_time = *(SpoorTime *)tmp;
}
