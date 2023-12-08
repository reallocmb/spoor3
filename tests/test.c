#include<eenheid.h>
#include<string.h>

#include<s_storage.c>
#include<s_time.c>
#include<s_object.c>
#include<s_span_time.c>
#include<s_sort.c>

__EENHEID_INIT__
SUITE(s_spoor_object.c)
{
    TEST(spoor_time_date_create())
    {
        char argument[50];
        SpoorTime spoor_time;
        SpoorTime spoor_time_today_test = {
            .day = 30,
            .mon = 0,
            .year = 123
        };

        test_spoor_time_today_set(&spoor_time_today_test);

        memcpy(argument, "5d", 2);
        argument[2] = 0;
        spoor_time_date_create(argument,
                               2,
                               &spoor_time);
        eenheid_assert_int32(spoor_time.day,
                             35);

        memcpy(argument, "1d", 2);
        spoor_time_date_create(argument,
                               2,
                               &spoor_time);
        eenheid_assert_int32(spoor_time.day,
                             31);

        TEST_END
    }
}
__EENHEID_END__
