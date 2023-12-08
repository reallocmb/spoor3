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

#endif
