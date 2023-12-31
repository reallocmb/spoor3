#include"spoor_internal.h"

#include<time.h>

#ifdef EENHEID_UNIT_TESTS

SpoorTime test_spoor_time_today;

void test_spoor_time_today_set(SpoorTime *spoor_time)
{
    test_spoor_time_today = *spoor_time;
}

SpoorTime spoor_time_today_get(time_t *time)
{
    return test_spoor_time_today;
}

#else

SpoorTime spoor_time_today_get(time_t *time)
{
   return *(SpoorTime *)localtime(time);
}

int32_t spoor_time_compare_day(SpoorTime *spoor_time0, SpoorTime *spoor_time1)
{
    return spoor_time0->day - spoor_time1->day + spoor_time0->mon - spoor_time1->mon + spoor_time0->year - spoor_time1->year;
}

#endif
