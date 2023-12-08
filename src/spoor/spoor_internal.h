#ifndef SPOOR_INTERNAL_H
#define SPOOR_INTERNAL_H

#include<time.h>
#include<stdint.h>
#include<stdbool.h>

typedef enum {
    STATUS_NOT_STARTED,
    STATUS_IN_PROGRESS,
    STATUS_COMPLETED,
} SpoorStatus;

typedef enum {
    TYPE_TASK,
    TYPE_PROJECT,
    TYPE_EVENT,
    TYPE_APPOINTMENT,
    TYPE_GOAL,
    TYPE_HABIT,
} SpoorType;

typedef struct {
    int32_t sec;
    int32_t min;
    int32_t hour;
    int32_t day;
    int32_t mon;
    int32_t year;
} SpoorTime;

typedef struct {
    SpoorTime start;
    SpoorTime end;
} SpoorTimeSpan;

typedef struct {
    char parent_id[100];
    char id[100];
} SpoorChild;

#define SPOOR_OBJECT_DELETED_ID 0xffffffff
#define SPOOR_OBJECT_NO_LINK 0xffffffff

typedef struct SpoorObject {
    uint32_t id;
    char title[250];
    char parent_title[250];
    SpoorTimeSpan deadline;
    SpoorTimeSpan schedule;
    SpoorTimeSpan tracked;
    SpoorTimeSpan complete;
    SpoorTimeSpan created;
    uint32_t id_link;
    SpoorStatus status;
    SpoorType type;
} SpoorObject;

typedef struct {
    uint32_t id;
    char location_parent[11];
    uint32_t id_parent;
    char location_child[11];
    uint32_t id_child;
    uint32_t id_prev;
    uint32_t id_next;
} SpoorLink;

extern SpoorLink link_global;

typedef struct {
    SpoorTimeSpan spoor_time;
} SpoorFilter;

typedef struct {
} SpoorSort;

SpoorObject *spoor_object_create(char *arguments);
bool spoor_object_edit(SpoorObject *spoor_object, char *arguments);

void mdb_func_error_check(int error, char *func_name, int line, char *file);
void spoor_debug_spoor_object_print(SpoorObject *spoor_object);

uint32_t spoor_object_storage_load(SpoorObject *spoor_objects, SpoorFilter *spoor_filter);
void spoor_sort_objects(SpoorObject *spoor_objects, uint32_t spoor_objects_count);

void spoor_object_progress_change(SpoorObject *spoor_object, SpoorStatus status);
void spoor_storage_save(SpoorObject *spoor_objects, SpoorObject *spoor_object);

void spoor_storage_change(SpoorObject *spoor_object);
void spoor_storage_delete(SpoorObject *spoor_object);
void spoor_object_schedule_set(SpoorObject *spoor_object, char *command);
void spoor_object_deadline_set(SpoorObject *spoor_object, char *command);

void spoor_storage_clean_up(void);
uint32_t spoor_object_storage_load_filter_time_span(SpoorObject *spoor_objects, SpoorTimeSpan *time_span);

void spoor_time_span_create(SpoorTimeSpan *spoor_time_span, char *command);

/* sort */
int64_t spoor_time_compare(SpoorTime *time1, SpoorTime *time2);
void spoor_sort_objects_by_title(SpoorObject *spoor_objects, uint32_t spoor_objects_count);
void spoor_sort_objects_by_deadline(SpoorObject *spoor_objects, uint32_t spoor_objects_count);

void spoor_time_deadline_create(char *argument, uint32_t argument_length, SpoorTimeSpan *spoor_time);

void storage_db_path_clean(SpoorObject *spoor_object, char *db_path_clean);
void spoor_object_children_append(SpoorObject *spoor_object_head, SpoorObject *spoor_object);
void spoor_object_children_append_edit(SpoorObject *spoor_object_head, SpoorObject *spoor_object, SpoorObject *old);
void spoor_storage_object_append(SpoorObject *spoor_object_parent, SpoorObject *spoor_object);
void spoor_storage_object_remove(SpoorObject *spoor_object);
void title_format_parse(char *title, char *title_format);
void time_format_parse_deadline(SpoorTimeSpan *spoor_time, char *time_format);
void time_format_parse_schedule(SpoorTimeSpan *spoor_time, char *time_format);
SpoorTime spoor_time_today_get(time_t *time);

/* for unit tests */
#ifdef EENHEID_UNIT_TESTS
void test_spoor_time_today_set(SpoorTime *spoor_time);
#endif

void spoor_link_load(uint32_t id);
void spoor_debug_links(void);

#endif
