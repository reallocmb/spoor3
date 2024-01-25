#include"eenheid/eenheid.h"

#include<spoor/s_time.c>

__EENHEID_INIT__
SUITE(s_spoor_object.c)
{
    TEST(spoor_time_date_create())
    {
        SpoorTime spoor_time0 = {
            .sec = 0,
            .min = 0,
            .hour = 0,
            .day = 31,
            .mon = 0,
            .year = 1900 + 124,
        };
        SpoorTime spoor_time1 = {
            .sec = 0,
            .min = 0,
            .hour = 0,
            .day = 0,
            .mon = 1,
            .year = 1900 + 124,
        };
        int32_t result = spoor_time_compare_day(&spoor_time0, &spoor_time1);
        eenheid_assert(result < 0);

        result = spoor_time_compare_day(&spoor_time1, &spoor_time0);
        eenheid_assert(result > 0);

        spoor_time1.mon = 0;
        spoor_time1.day = 31;
        result = spoor_time_compare_day(&spoor_time1, &spoor_time0);
        eenheid_assert(result == 0);

    } TEST_END
}
__EENHEID_END__
