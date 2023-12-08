#include"spoor_internal.h"
#include"../redbas/redbas.h"

#include<stdio.h>
#include<string.h>

const char DEBUG_TYPES[][17] = {
    "TYPE_TASK",
    "TYPE_PROJECT",
    "TYPE_EVENT",
    "TYPE_APPOINTMENT",
    "TYPE_TARGET",
    "TYPE_HABIT"
};

const char DEBUG_STATUS[][19] = {
    "STATUS_NOT_STARTED",
    "STATUS_IN_PROGRESS",
    "STATUS_COMPLETED"
};

void spoor_time_hour_minute_format(char *format_buffer, SpoorTime *time)
{
    if (time->hour == -1 || time->min == -1)
        sprintf(format_buffer, "--:--");
    else
    {
        sprintf(format_buffer, "%s%d:%s%d",
                (time->hour < 10) ?"0" :"", time->hour,
                (time->min < 10) ?"0" : "", time->min);
    }
}

void spoor_time_year_format(char *format_buffer, int year)
{
    if (year == -1)
        sprintf(format_buffer, "----");
    else
    {
        sprintf(format_buffer, "%d", year + 1900);
    }
}

void spoor_time_mon_format(char *format_buffer, int mon)
{
    if (mon == - 1)
        sprintf(format_buffer, "--");
    else
    {
        sprintf(format_buffer, "%s%d",
                (mon < 10) ?"0" :"", mon + 1);
    }
}

void spoor_time_day_format(char *format_buffer, int day)
{
    if (day == -1)
        sprintf(format_buffer, "--");
    else
        sprintf(format_buffer, "%s%d",
                (day < 10) ?"" :"", day);
}

void debug_spoor_time_print(SpoorTimeSpan *spoor_time)
{
    char format_buffer_hour_min_start[6];
    char format_buffer_hour_min_end[6];
    char format_buffer_year_start[5];
    char format_buffer_year_end[5];
    char format_buffer_mon_start[3];
    char format_buffer_mon_end[3];
    char format_buffer_day_start[3];
    char format_buffer_day_end[3];

    spoor_time_hour_minute_format(format_buffer_hour_min_start, &spoor_time->start);
    spoor_time_hour_minute_format(format_buffer_hour_min_end, &spoor_time->end);

    spoor_time_year_format(format_buffer_year_start, spoor_time->start.year);
    spoor_time_year_format(format_buffer_year_end, spoor_time->end.year);

    spoor_time_mon_format(format_buffer_mon_start, spoor_time->start.mon);
    spoor_time_mon_format(format_buffer_mon_end, spoor_time->end.mon);

    spoor_time_day_format(format_buffer_day_start, spoor_time->start.day);
    spoor_time_day_format(format_buffer_day_end, spoor_time->end.day);

    printf("%s.%s.%s %s - %s.%s.%s %s\n",
            format_buffer_day_start,
            format_buffer_mon_start,
            format_buffer_year_start,
            format_buffer_hour_min_start,
            format_buffer_day_end,
            format_buffer_mon_end,
            format_buffer_year_end,
            format_buffer_hour_min_end);
}

void spoor_debug_spoor_object_print(SpoorObject *spoor_object)
{
    printf("[ SpoorObject --\n");
    printf("%-20s%d\n", "Key:", spoor_object->id);
    printf("%-20s%s\n", "Title:", spoor_object->title);
    printf("%-20s%s\n", "Status:", DEBUG_STATUS[spoor_object->status]);
    printf("%-20s", "Deadline: ");
    debug_spoor_time_print(&spoor_object->deadline);
    printf("%-20s", "Schedule: ");
    debug_spoor_time_print(&spoor_object->schedule);
    printf("%-20s", "Tracked: ");
    debug_spoor_time_print(&spoor_object->tracked);
    printf("%-20s", "Complete: ");
    debug_spoor_time_print(&spoor_object->complete);
    printf("%-20s", "Created: ");
    debug_spoor_time_print(&spoor_object->created);
    printf("%-20s%s\n","Status:", DEBUG_STATUS[spoor_object->status]);
    printf("%-20s%s\n", "Type:", DEBUG_TYPES[spoor_object->type]);
    printf("%-20s%s\n", "Parent-Title:", spoor_object->parent_title);
    printf("%-20s%d\n", "Link-ID:", spoor_object->id_link);
    if (spoor_object->id_link != SPOOR_OBJECT_NO_LINK)
    {
        spoor_link_load(spoor_object->id_link);
        printf("%-20s%d\n", "LG-id:", link_global.id);
        printf("%-20s%d\n", "LG-id_parent:", link_global.id_parent);
        printf("%-20s%s\n", "LG-location_parent:", link_global.location_parent);
        printf("%-20s%d\n", "LG-id_child:", link_global.id_child);
        printf("%-20s%s\n", "LG-location_child:", link_global.location_child);
        printf("%-20s%d\n", "LG-id_prev:", link_global.id_prev);
        printf("%-20s%d\n", "LG-id_next:", link_global.id_next);
    }
    printf("-- SpoorObject ]\n");
}

void spoor_debug_links(void)
{
    RedbasDB *db = redbas_db_open("links", sizeof(link_global));
    uint32_t items = redbas_db_items(db);

    uint32_t i;

    for (i = 0; i < items; i++)
    {
        redbas_db_restore_cursor_set(db, i);
        redbas_db_restore(db, &link_global, sizeof(link_global));
        puts("[");
        printf("%-20s%d\n", "LG-id:", link_global.id);
        printf("%-20s%d\n", "LG-id_parent:", link_global.id_parent);
        printf("%-20s%s\n", "LG-location_parent:", link_global.location_parent);
        printf("%-20s%d\n", "LG-id_child:", link_global.id_child);
        printf("%-20s%s\n", "LG-location_child:", link_global.location_child);
        printf("%-20s%d\n", "LG-id_prev:", link_global.id_prev);
        printf("%-20s%d\n", "LG-id_next:", link_global.id_next);
        puts("]");
    }
    redbas_db_close(db);

}
