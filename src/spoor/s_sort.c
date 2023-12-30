#include"spoor_internal.h"
#include<stdlib.h>

int64_t spoor_time_compare(SpoorTime *time1, SpoorTime *time2)
{
    int64_t result = 0;
    result += ((time1->year != -1) * time1->year - (time2->year != -1) * time2->year) * 60 * 24 * 31 * 12;
    result += ((time1->mon != -1) * time1->mon - (time2->mon != -1) * time2->mon) * 60 * 24 * 31;
    result += ((time1->day != -1) * time1->day - (time2->day != -1) * time2->day) * 60 * 24;
    result += ((time1->hour != -1) * time1->hour - (time2->hour != -1) * time2->hour) * 60;
    result += ((time1->min != -1) * time1->min - (time2->min != -1) * time2->min);

    return result;
}

int sort_func(const void *data0, const void *data1)
{
    int32_t result = spoor_time_compare(&((SpoorObject *)data0)->schedule.end, &((SpoorObject *)data1)->schedule.end);
    if (result == 0)
        result = spoor_time_compare(&((SpoorObject *)data0)->deadline.start, &((SpoorObject *)data1)->deadline.start);

    return result;
}

void spoor_sort_objects_by_deadline(void)
{
    qsort(spoor_objects, spoor_objects_count, sizeof(*spoor_objects), sort_func);
}

void spoor_sort_objects(void)
{

    uint32_t i;
    SpoorObject temp;
    while (spoor_objects_count--)
    {
       for (i = 1; i <= spoor_objects_count; i++)
        {
            if (spoor_time_compare(&spoor_objects[i - 1].deadline.start, &spoor_objects[i].deadline.start) == 1)
            {
                temp = spoor_objects[i - 1];
                spoor_objects[i - 1] = spoor_objects[i];
                spoor_objects[i] = temp;
            }
        }
    }
}
