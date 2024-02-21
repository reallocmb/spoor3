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
    int32_t result = spoor_time_compare(&((SpoorObject *)data0)->schedule.start, &((SpoorObject *)data1)->schedule.start);
    if (result == 0)
        result = spoor_time_compare(&((SpoorObject *)data0)->deadline.end, &((SpoorObject *)data1)->deadline.end);
    if (result == 0)
        result = ((SpoorObject *)data0)->status - ((SpoorObject *)data1)->status;

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

uint32_t spoor_sort_objects_reposition_up(uint32_t index)
{
    if (index == 0)
        return index;

    uint32_t i = index;
    while (--i)
    {
        if (sort_func((void *)&spoor_objects[i], (void *)&spoor_objects[index]) > 0)
        {
            SpoorObject tmp;
            tmp = spoor_objects[i];
            spoor_objects[i] = spoor_objects[index];
            spoor_objects[index] = tmp;
            index = i;
        }
        else
            break;
    }

    return index;
}

uint32_t spoor_sort_objects_reposition_down(uint32_t index)
{
    if (index == spoor_objects_count - 1)
        return index;

    uint32_t i;
    for (i = index + 1; i < spoor_objects_count; i++)
    {
        if (sort_func((void *)&spoor_objects[index], (void *)&spoor_objects[i]) > 0)
        {
            SpoorObject tmp;
            tmp = spoor_objects[i];
            spoor_objects[i] = spoor_objects[index];
            spoor_objects[index] = tmp;
            index = i;
        }
        else
            break;
    }
    return index;
} 

void spoor_sort_objects_append(SpoorObject *spoor_object)
{
    uint32_t i;
    for (i = 0; i < spoor_objects_count; i++)
    {
        if (spoor_objects[i].schedule.start.hour != -1 &&
            spoor_objects[i].schedule.end.hour != -1)
            break;
    }

    for (; i < spoor_objects_count; i++)
    {
        if (sort_func((void *)spoor_object, (void *)&spoor_objects[i]) <= 0)
        {
            SpoorObject tmp;

            for (; i < spoor_objects_count; i++)
            {
                tmp = spoor_objects[i];
                spoor_objects[i] = *spoor_object;
                *spoor_object = tmp;
            }
        }
    }

    spoor_objects[spoor_objects_count++] = *spoor_object;
}

void spoor_sort_objects_remove(uint32_t index)
{
    uint32_t i;
    for (i = index; i < spoor_objects_count - 1; i++)
        spoor_objects[i] = spoor_objects[i + 1];

    spoor_objects_count--;
}
